/*
 * gridcad -- a tool to do digital circuit design with a GUI
 *
 * (C) Copyright 2025  -- Arjan van de Ven
 *
 * Licensed under the terms of the
 * GNU General Public LIcense version 3.0
 *
 * Authors:
 *    Arjan van de Ven <arjanvandeven@gmail.com>
 */

#include "gridcad.h"
#include "wire.h"
#include "port.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

void callback_set_color(class port *port, int color)
{
	printf("Pushing color %i \n", color);
	port->color = color;
	port->push_wire_color(color);
}

port::port(std::string _name, int _direction, int _bus_width)
{

	direction = _direction;
	name = _name;
	bus_width = _bus_width;

	if (direction == PORT_OUT) {
		menu = new class port_contextmenu(this);	
		menu->add_item(wire_color_name(0), 0, callback_set_color);
		menu->add_item(wire_color_name(1), 1, callback_set_color);
		menu->add_item(wire_color_name(2), 2, callback_set_color);
		menu->add_item(wire_color_name(3), 3, callback_set_color);
		menu->add_item(wire_color_name(4), 4, callback_set_color);
		menu->add_item(wire_color_name(5), 5, callback_set_color);
		menu->add_item(wire_color_name(6), 6, callback_set_color);
		menu->add_item(wire_color_name(7), 7, callback_set_color);
	} else {
		menu = NULL;
	}
}

port::~port()
{
	if (menu) {
		delete menu;
		menu = NULL;
	}
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
	
	if (wire->get_width() == 0) 
		wire->set_width( get_width());
		
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
	if (direction == PORT_OUT)
		wire->push_wire_color(color);
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
//        if (type != DRAW_GHOST && type != DRAW_DND)
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
	std::string icon = "";
	if (direction == PORT_IN) {
		if (wires.size() == 0)
			icon = "assets/port_in_open.png";
		else
			icon = "assets/port_in_connected.png";
	} else {
		if (value.boolval)
			icon = "assets/port_out_green.png";
		else
			icon = "assets/port_out_red.png";

		if (value.type == VALUE_TYPE_INT || bus_width > 1)
			icon = "assets/port_out_bus.png";
	}
	canvas->draw_image(icon, cX + X, cY + Y, 1, 1, Alpha(type));		
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
	j["color"] = color;
	
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
	color = j.value("color", 0);
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
	while (wires.size() > 0) {
		auto wire = wires[0];
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

void port::unsplice(void)
{
	if (wires.size() != 2)
		return;
	class wire *w1, *w2, *w3;
	w1 = wires[0];
	w2 = wires[1];
	class port *p1, *p2;
	
	w3 = new class wire(0,0,0,0);
		
	p1 = w1->get_other_port(this);
	p2 = w2->get_other_port(this);
	
	if (p1) {
		w3->add_port(p1);
		p1->add_wire(w3);
	} else {
		printf("NO P1\n");
	}
	if (p2) {
		w3->add_port(p2);
		p2->add_wire(w3);
	} else {
		printf("NO P1\n");
	}
	w1->remove();
	w2->remove();
	w3->reseat();
	
}


void port::push_wire_color(int color)
{
//	if (!is_connector)  /* only connectors will propagate wire colors */
//		return;
	for (auto wire : wires)
		wire->push_wire_color(color);
}
