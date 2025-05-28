#include "gridcad.h"
#include "wire.h"
#include "port.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

port::port(std::string _name, int _direction, int _bus_width)
{

	direction = _direction;
	name = _name;
	bus_width = _bus_width;
	
}

port::~port()
{
	while (wires.size() > 0) {
		class wire *wire = wires[0];
		remove_wire(wire);
		if (wire->is_empty())
			delete wire;
	}
}


void port::add_wire(class wire * wire)
{
	for (auto _wire : wires) {
		if (wire == _wire)
			return;
	}
	wires.push_back(wire);
	
	if (direction == PORT_OUT) {
		wire->update_value(&value, DEFAULT_TTL);
	}
	wire->add_port(this);
	
	if (bus_width) {
		wire->set_width(bus_width);
	}
	
	if (direction == PORT_IN) {
		update_value(&(wire->value), DEFAULT_TTL);
	}
	if (parent)
		parent->notify(DEFAULT_TTL);
	wire->notify(DEFAULT_TTL);
}

void port::update_value(struct value *newvalue, int ttl)
{
	if (ttl <= 0)
		return;
	if (!newvalue->valid)
		return;
	if (memcmp(&value, newvalue, sizeof(struct value)) == 0)
		return;
		
	value = *newvalue;

	for (auto wire:wires) {
		wire->update_value(newvalue, ttl -1);
	}
	
//	notify(ttl -1);
	if (parent && direction != PORT_OUT)
		parent->notify(ttl -1);
}



void port::drawAt(class canvas * canvas, float _X, float _Y, int type)
{
        if (type != DRAW_GHOST && type != DRAW_DND)
		draw_wires(canvas);
	if (direction == PORT_IN) {
		drawConnector(canvas, _X, _Y, X, Y, COLOR_ELEMENT_CONNECTOR + type);
	} else { 
		drawConnector(canvas, _X, _Y, X, Y, value_color(&value));
	}
}

void port::drawAt2(class canvas * canvas, float _X, float _Y, int type)
{
	if (direction == PORT_IN) {
		drawConnector(canvas, _X, _Y, X, Y, COLOR_ELEMENT_CONNECTOR + type);
	} else { 
		drawConnector(canvas, _X, _Y, X, Y, value_color(&value));
	}
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
	if (direction == PORT_IN) {
		if (wires.size() == 0)
			canvas->draw_image("assets/port_in_open.png", cX + X, cY + Y, 1, 1, Alpha(type));		
		else
			canvas->draw_image("assets/port_in_connected.png", cX + X, cY + Y, 1, 1, Alpha(type));		
	} else {
		if (value.boolval)
			canvas->draw_image("assets/port_out_green.png", cX + X, cY + Y, 1, 1, Alpha(type));		
		else
			canvas->draw_image("assets/port_out_red.png", cX + X, cY + Y, 1, 1, Alpha(type));		
	}
        canvas->draw_text(name, cX + X, cY + Y + 0.35, 1, 0.3);
}

void port::stop_drag(class canvas *canvas)
{
    for (auto wire : wires) {
        wire->reseat();
        wire->route(canvas->get_scene());
    }

}

void port::notify(int ttl)
{	
    if (ttl <= 0)
    	return;
    if (direction == PORT_IN)
    	return;
    for (auto wire : wires) {
    	wire->update_value(&value, ttl -1);
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
bool port::replace_wire(class wire *from, class wire *to)
{
	bool result = true;
#if 0	
	for (unsigned int i = 0; i< wires.size(); ++i) {
		class wire *wire = wires[i];
		if (wire == from) {
			wires[i] = to;
			to->add_port(this);
			result = true;
		}
	}
#endif
	remove_wire(from);
	add_wire(to);	
	return result;
}


void port::to_json(json &j)
{
	unsigned int i;
	j["X"] = X;
	j["Y"] = Y;
	j["screenX"] = screenX;
	j["screenY"] = screenY;
	j["name"] = name;
	j["value"] = value;
	j["direction"] = direction;
	j["bus_width"] = bus_width;
	j["linked_uuid"] = linked_uuid;
	
	j["wires"] = json::array();
	for (i = 0; i < wires.size(); i++) {
		json p;
		wires[i]->to_json(p);
		j["wires"][i] = p;
	}
}

void port::from_json(json &j)
{
	unsigned int i;
	X = j["X"];
	Y = j["Y"];
	screenX = j["screenX"];
	screenY = j["screenY"];
	name = j["name"];
	value = j["value"];
	direction = j["direction"];
	bus_width = j.value("bus_width", 1);
	linked_uuid = j.value("linked_uuid", "");
	for (i = 0; i < j["wires"].size(); i++) {
		class wire *wire;
		wire = json_wire_factory(j["wires"][i]);
		add_wire(wire);
		wire->add_port(this);
		wire->reseat();
	}

}

void port::remove_wires(void)
{
	for (auto wire : wires) {
		wire->remove();
	}
	wires.clear();
}

void port::fill_grid(class wiregrid *grid)
{
	for (auto wire : wires) {
		wire->fill_grid(grid);
	}
}

void port::remove_orphans(void)
{
	for (auto wire : wires) {
		wire->remove_if_orphan();
	}
}

void port::update_name(std::string newname)
{
	name = newname;
}

void port::link_uuid(std::string _uuid)
{
	linked_uuid = _uuid;
}

void port::route_wires(void)
{
	for (auto wire : wires) {
		wire->reseat();
		wire->redo_wires();
	}
}

int port::get_width(void)
{
	int width = 0;
	if (bus_width)
		return bus_width;
	for (auto wire : wires) {
		width = std::max(width, wire->get_width());
	}
	return width;
}