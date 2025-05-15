#include "gridcad.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

port::port(const char *_name, int _direction)
{

	direction = _direction;
	name = strdup(_name);
	
}

port::~port()
{
}


void port::add_wire(class wire * wire)
{
	wires.push_back(wire);
	if (direction == PORT_OUT) {
		wire->update_value(&value);
	}
	wire->add_port(this);
	
	if (direction == PORT_IN) {
		update_value(&(wire->value));
	}
	if (parent)
		parent->notify();
	wire->notify();
}

void port::update_value(struct value *newvalue)
{
	if (memcmp(&value, newvalue, sizeof(struct value)) == 0)
		return;
	for (auto wire:wires) {
		wire->update_value(newvalue);
	}
	value = *newvalue;
	
	notify();
	if (parent)
		parent->notify();
}



void port::drawAt(class canvas * canvas, float _X, float _Y, int type)
{
	if (!label) {
		label = canvas->text_to_texture(name);
	}

	if (direction == PORT_IN) {
		drawConnector(canvas, _X, _Y, X, Y, COLOR_ELEMENT_CONNECTOR + type);
	} else { 
		drawConnector(canvas, _X, _Y, X, Y, value_color(&value));
	}
	draw_wires(canvas);
}

void port::draw_wires(class canvas * canvas)
{
	for (auto wire : wires) {
        	wire->draw(canvas);
	}
}

void port::draw(class canvas *canvas, int color)
{
	drawAt(canvas, screenX, screenY, color);
}


void port::drawConnector(class canvas * canvas, float X, float Y, int cX, int cY, int type)
{
	canvas->drawCircle(cX + X + 0.5, cY + Y + 0.5, 0.51, type);
	if (label) {
		double w,h;
		SDL_Point size;
		SDL_QueryTexture(label, NULL, NULL, &size.x, &size.y);
		
		h = 0.3;
		w = size.x / size.y * h;
		
		canvas->draw_image(label, cX + X + (1 - w)/2, cY + Y + 1, w, h); 
	}
}

void port::stop_drag(class canvas *canvas)
{
    for (auto wire : wires) {
        wire->reseat();
        wire->route(canvas->get_scene());
    }

}

void port::notify(void)
{
    if (direction == PORT_IN)
    	return;
    for (auto wire : wires) {
    	wire->update_value(&value);
    }
}

class wire *port::is_wire(float X, float Y)
{
	for (auto wire : wires) {
		if (wire->intersect(X, Y))
			return wire;	
	}
	return NULL;
}

void port::remove_wire(class wire *_wire)
{
	for (unsigned int i = 0; i< wires.size(); ++i) {
		class wire *wire = wires[i];
		if (wire == _wire) {
			wires.erase(wires.begin() + i);
			return;
		}
	}
}