#include "gridcad.h"

port::port(int _direction)
{
	direction = _direction;
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
}

void port::update_value(struct value *newvalue)
{
	if (memcmp(&value, newvalue, sizeof(struct value)) == 0)
		return;
	for (auto wire:wires) {
		wire->update_value(newvalue);
	}
}



void port::drawAt(class scene * scene, float _X, float _Y, int type)
{
	if (direction == PORT_IN) {
		drawConnector(scene, _X, _Y, X, Y, COLOR_ELEMENT_CONNECTOR + type);
	} else { 
		drawConnector(scene, _X, _Y, X, Y, value_color(&value));
	}

}

void port::draw(class scene *scene, int color)
{
	drawAt(scene, screenX, screenY, color);
}


void port::drawConnector(class scene * scene, float X, float Y, int cX, int cY, int type)
{
	scene->drawCircle(cX + X + 0.5, cY + Y + 0.5, 0.48, type);
}
