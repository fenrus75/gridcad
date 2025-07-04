#include "gridcad.h"
#include "seqcanvas.h"
#include "sequencer.h"
#include "port.h"

void callback_reset_sequencer(class element *elmnt)
{
	class sequencer * seq = (class sequencer *)elmnt;
	seq->reset_pointer();
}

void callback_stop_error(class element *elmnt)
{
	class sequencer * seq = (class sequencer *)elmnt;
	seq->toggle_stop_error();
	queue_calculate(seq);
}


sequencer::sequencer(int X, int Y) : element(X, Y, "Sequencer")
{
	sizeX = 3;
	sizeY = 4;

	add_port(1, sizeY, "clk", PORT_IN, 1);
	add_port(sizeX, 1, "Out", PORT_OUT);
	add_port(-1, 1, "In", PORT_IN);

	menu->add_item("Edit name", callback_editname);
	menu->add_item("Reset position", callback_reset_sequencer);
	menu->add_item("Toggle 'stop on error'", callback_stop_error);
	name_edit = new class name(&name);
}

sequencer::~sequencer(void)
{
	delete name_edit;
	delete canvas;
}


void sequencer::add_value_row(void)
{
	values.push_back({.valid = true});
}

void sequencer::remove_value_row(void)
{
	values.pop_back();
}

void sequencer::to_json(json &j)
{
	element::to_json(j);
	j["values"] = values;
	j["current_value"] = current_value;
	j["current_clock"] = current_clock;
	j["is_error"] = is_error;
	j["stop_clock_on_error"] = stop_clock_on_error;
}

void sequencer::from_json(json &j)
{
	element::from_json(j);
	values = j["values"];
	current_value = j.value("current_value", 0);
	current_clock = j.value("current_clock", false);
	is_error = j.value("is_error", false);
	stop_clock_on_error = j.value("stop_clock_on_error", false);
}

void sequencer::handle_event(class basecanvas *canvas, SDL_Event &event)
{
	element::handle_event(canvas, event);
	name_edit->handle_event(event);
}


void sequencer::drawAt(class basecanvas *canvas, float X, float Y, int type)
{
	if (selected) {
		canvas->draw_image("assets/sequencer_selected.png", X, Y, sizeX, sizeY, Alpha(type));
	} else {	
		if (is_error)
			canvas->draw_image("assets/sequencer_error.png", X, Y, sizeX, sizeY, Alpha(type));
		else
			canvas->draw_image("assets/sequencer.png", X, Y, sizeX, sizeY, Alpha(type));
	}

	if (values.size() > 0) {
		canvas->draw_text(std::to_string(values[current_value].intval), X+1, Y+1, 1,1);
	}
	if (ports[0]->has_net())
		canvas->draw_text(std::to_string(current_value), X+1.75, Y+3, 1.25, 1); 

	name_edit->drawAt(canvas,X, Y - 1, sizeX);

	hover_ports(canvas);

	for (auto port:ports) {
		port->drawAt(canvas, X, Y, COLOR_WIRE_SOLID);
		port->draw_wires(canvas);
	}
}

bool sequencer::mouse_select(float _X, float _Y)
{
	long int click = time(NULL);

	if (click - previous_click > 1) {
		previous_click = click;
		return false;
	}	

	if (!canvas) {
		printf("Spawning a new window\n");
		canvas = new class seqcanvas(this);
		register_new_canvas(canvas);	
	} else {
		canvas->unhide();
	}
	previous_click = click;
	return false;
}

void sequencer::calculate(int ttl)
{
	struct value newclock;
	newclock = ports[0]->value;

	check_input();

	if (newclock.boolval == current_clock)
		return;

	if (newclock.boolval) { /* rising edge */
		current_value++;
		if (current_value >= values.size())
			current_value = 0;
		queue_calculate(this); /* schedule calculations for the propagation */
	} else {
		/* falling edge */
		if (is_error && stop_clock_on_error) {
			SDL_Event ev = {};
			ev.type = EVENT_STOP_CLOCK;
			ev.user.code = 0;
			SDL_PushEvent(&ev);
		}
	};

	current_clock = newclock.boolval;
}

void sequencer::queued_calculate(int ttl)
{
	if (values.size() <= current_value)
		return;
	if (ports[1]->get_net_width() > 1)
		values[current_value].type = VALUE_TYPE_INT;
	update_value_net(&values[current_value], 1, ttl - 1);	

	check_input();
}

void sequencer::check_input(void)
{
	if (values.size() <= current_value)
		return;
	if (ports[2]->has_net()) {
		is_error = false;
		if (ports[2]->get_net_width() > 1) {
			if (ports[2]->value.intval != values[current_value].intval) 
				is_error = true;
		} else {
			if (ports[2]->value.boolval != values[current_value].boolval) 
				is_error = true;
		}
	}

}



std::string sequencer::get_verilog_main(void)
{
	std::string s = "";
	if (verilog_module_name == "")
		verilog_module_name = append_random_bits(verilog_name + "_tt_");

	s = s + verilog_module_name + " " + get_verilog_name() + "(";
	bool first = true;
	for (auto port : ports) {

		if (!port->has_net())
			continue;

		if (!first)
			s = s + ", ";
		first = false;
		s = s + "." + port->get_verilog_name() + "(";
		s = s + port->get_net_verilog_name();
		s = s + ")";

	}
	s = s + ");\n";

	return s;
}

#define BIT_IS_SET(bit, value)  ( !!((1 << bit) & value))

static unsigned int highest_addr_bit(unsigned int size)
{
    unsigned int i,j;
    i = 0;
    j = 2;
    while (1) {
        if (size <= j) {
            return i;
        }
        j = j << 1;
        i ++;
    }
    return 0;
}


/* TODO -- rather than a pure basic SOP, run espresso on this and make it more minimal */
std::string sequencer::get_verilog_modules(std::string path)
{
	std::string s = "";
	
	unsigned int outwidth = ports[1]->get_net_width();
	unsigned int counterwidth = highest_addr_bit(values.size());

	s = s + "module " + verilog_module_name + "\n (";
	bool first = true;
	for (auto port : ports) {
		if (!port->has_net())
			continue;
		if (!first)
			s = s + ", ";
		first = false;
		if (port->direction == PORT_IN)
			s = s + "input ";
		if (port->direction == PORT_OUT)
			s = s + "output ";
		s = s + port->get_verilog_width() + " ";
		s = s +port->get_verilog_name();

	}
	s = s + ");\n\n";
	
	s = s + "reg [" + std::to_string(counterwidth) + ":0] counter;\n";
	s = s + "wire [" + std::to_string(counterwidth) + ":0] nextcounter;\n";
	
	for (unsigned int i = 0; i < outwidth; i++) {
		first = true;
		s = s +"assign Out[" + std::to_string(i) + "] = ";

		for (unsigned int Y = 0; Y < values.size(); Y++) {
			if (BIT_IS_SET(i, Y)) {
				if (!first) 
					s = s + " | ";
				first = false;
				s = s + "(";
				bool nestedfirst = true;
				for (unsigned int x = 0; x <= counterwidth; x++) {
					if (BIT_IS_SET(x, values[Y].intval)) {
						if (!nestedfirst)
							s = s + " & ";
						nestedfirst = false;
						s = s + "counter[" + std::to_string(x) +"]";
					} else {
						if (!nestedfirst)
							s = s + " & ";
						nestedfirst = false;
						s = s + "(! counter[" +std::to_string(x) + "])";
					}
				}

				s = s + ")";
			}
		}

		if (first) 
			s = s + "1'b0";

		s = s + ";\n";
	}

	for (unsigned int i = 0; i <= counterwidth; i++) {
		first = true;
		s = s +"assign nextcounter[" + std::to_string(i) + "] = ";

		for (unsigned int Y = 0; Y < values.size(); Y++) {
			unsigned int nextc = Y + 1;
			if (nextc >= values.size())
				nextc  = 0;
			if (BIT_IS_SET(i, Y)) {
				if (!first) 
					s = s + " | ";
				first = false;
				s = s + "(";
				bool nestedfirst = true;
				for (unsigned int x = 0; x <= counterwidth; x++) {
					if (BIT_IS_SET(x, nextc)) {
						if (!nestedfirst)
							s = s + " & ";
						nestedfirst = false;
						s = s + "counter[" + std::to_string(x) +"]";
					} else {
						if (!nestedfirst)
							s = s + " & ";
						nestedfirst = false;
						s = s + "(! counter[" +std::to_string(x) + "])";
					}
				}

				s = s + ")";
			}
		}

		if (first) 
			s = s + "1'b0";

		s = s + ";\n";
	}
	s = s + "\n";
	s = s + "always @(posedge clk) \n";
	s = s + "begin\n";
	s = s + "    counter <= nextcounter;\n";
	s = s + "end \n";

	s = s + "endmodule\n";


	return s;
}


std::string sequencer::get_verilog_name(void)
{
	if (verilog_name == "") {
		verilog_name = name; 
		std::replace(verilog_name.begin(), verilog_name.end(), ' ', '_');
		std::replace(verilog_name.begin(), verilog_name.end(), '-', '_');
		std::replace(verilog_name.begin(), verilog_name.end(), '+', '_');
		verilog_name = append_random_bits(verilog_name + "_");
	}


	return verilog_name;
}

