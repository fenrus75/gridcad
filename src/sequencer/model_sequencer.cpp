#include "gridcad.h"
#include "seqcanvas.h"
#include "sequencer.h"
#include "port.h"

sequencer::sequencer(int X, int Y) : element(X, Y, "Sequencer")
{
	sizeX = 3;
	sizeY = 4;

	add_port(1, sizeY, "clk", PORT_IN, 1);
	add_port(sizeX, 1, "Out", PORT_OUT);

	menu->add_item("Edit name", callback_editname);
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
}

void sequencer::from_json(json &j)
{
	element::from_json(j);
	values = j["values"];
	current_value = j.value("current_value", 0);
	current_clock = j.value("current_clock", false);
}

void sequencer::handle_event(class basecanvas *canvas, SDL_Event &event)
{
}


void sequencer::drawAt(class basecanvas *canvas, float X, float Y, int type)
{
	if (selected) {
        	canvas->draw_image("assets/sequencer_selected.png", X, Y, sizeX, sizeY, Alpha(type));
	} else {	
	        canvas->draw_image("assets/sequencer.png", X, Y, sizeX, sizeY, Alpha(type));
	}
    
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
     
     if (newclock.boolval == current_clock)
       return;
       
     if (newclock.boolval) { /* rising edge */
     	  current_value++;
     	  if (current_value >= values.size())
     	  	current_value = 0;
	  queue_calculate(this); /* schedule calculations for the propagation */
     };
     
     current_clock = newclock.boolval;
}

void sequencer::queued_calculate(int ttl)
{
	if (values.size() <= current_value)
		return;
	printf("Current value %i \n", current_value);
	printf("sizeof values %lu\n", values.size());
	if (ports[1]->get_net_width() > 1)
		values[current_value].type = VALUE_TYPE_INT;
	update_value_net(&values[current_value], 1, ttl - 1);	
}