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
}

void sequencer::from_json(json &j)
{
	element::from_json(j);
	values = j["values"];
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
