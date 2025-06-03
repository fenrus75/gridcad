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

#include "model_nest.h"
#include "model_toggle.h"
#include "model_output.h"
#include "port.h"
#include <algorithm>
#include <sys/time.h>
#include <nlohmann/json.hpp>
#include "contextmenu.h"
#include "name.h"

static float dist(float X1, float Y1, float X2, float Y2)
{
	return sqrtf( (X2-X1)*(X2-X1) + (Y2-Y1)*(Y2-Y1));
}

model_nest::model_nest(float _X, float _Y) : element(_X,_Y, "SubScreen")
{
	sizeX = 4;
	sizeY = 4;
	X = floorf(_X);
	Y = floorf(_Y);    
	_scene = new class scene(name);
	menu->add_item("Edit name", callback_editname);
	
	name_edit = new class name(&name);
}

model_nest::~model_nest(void)
{
	delete name_edit;
	if (canvas)
		delete canvas;
	else /* canvas destructor already deleted _scene, but there might not be a canvas yet */
		delete _scene;
		
}


void model_nest::drawAt(class canvas *canvas, float X, float Y, int type)
{
	maybe_regen_ports();
        if (!selected)
    	  name_edit->set_edit_mode(false);
	if (!selected) {
		if (icon == "") {
			canvas->draw_image("assets/nest/nest_back.png", X, Y, sizeX, sizeY, Alpha(type));
			canvas->draw_image("assets/nest/icon.png", X+0.15, Y+0.15, sizeX-0.3, sizeY-0.3, Alpha(type), true);	
		} else {
			canvas->draw_image(icon, X, Y, sizeX, sizeY, Alpha(type));
		}
	} else {
		if (icon_selected == "") 
			canvas->draw_image("assets/nest/nest_selected.png", X, Y, sizeX, sizeY, Alpha(type));
		else {
			canvas->draw_image(icon_selected, X, Y, sizeX, sizeY, Alpha(type));
		}
	}

	name_edit->drawAt(canvas, X, Y + sizeY, sizeX);

	for (auto port: ports) {
        	port->drawAt(canvas, X, Y, type);
	}

	hover_ports(canvas);     	 
	
}


void model_nest::calculate(int ttl)
{
	maybe_regen_ports();
	for (auto port: ports) {
		if (port->direction == PORT_IN) {
			class model_toggle *tog;
			tog = (class model_toggle *)port->get_linked_element();
			if (tog) {
				tog->update_value(&(port->value), ttl-1);
				port->set_width(tog->get_width());
			}
		}
	}
	for (auto port: ports) {
		if (port->direction == PORT_OUT) {
			class model_output *tog;
			struct value val = {};
			tog = (class model_output *)port->get_linked_element();
			if (tog) {
				val = tog->get_value();
				port->update_value(&val, ttl-1);
				port->set_width(tog->get_width());
			}
		}
	}
}


bool model_nest::mouse_select(float _X, float _Y)
{
	long int click = time(NULL);
	
	regen_ports();

	if (click - previous_click > 1) {
		previous_click = click;
		return false;
	}	
	
	if (!canvas) {
		printf("Spawning a new window\n");
		canvas = new class canvas(_scene);
		register_new_canvas(canvas);	
	} else {
		canvas->unhide();
	}
	previous_click = click;
	return false;
}


void model_nest::handle_event(class canvas *thiscanvas, SDL_Event &event)
{
    switch (event.type) {
	case SDL_MOUSEMOTION:
		mouseX = thiscanvas->scr_to_X(event.motion.x); 
	        mouseY = thiscanvas->scr_to_Y(event.motion.y);

		break;
    }
    if (!selected || !single)
        return;
    switch (event.type) {
	case SDL_KEYDOWN:        
        	switch (event.key.keysym.sym) {
                case SDLK_RETURN:
                    name_edit->toggle_edit_mode();
                    break;
                }
                break;
    }
    if (name_edit)
      name_edit->handle_event(event);
}

void model_nest::to_json(json &j)
{
     json p;
     element::to_json(j);
     if (!canvas)
	     _scene->to_json(p);
     else
     		canvas->get_scene()->to_json(p);
     j["scene"] = p;   
     j["icon"] = icon;
     j["icon_selected"] = icon_selected;
}
void model_nest::from_json(json &j)
{
     element::from_json(j);
     if (!canvas)
	     _scene->from_json(j["scene"]);
     else
     	canvas->get_scene()->from_json(j["scene"]);
     	
     icon = j.value("icon", "");
     icon_selected = j.value("icon_selected", "");
     regen_ports();
}

static bool compare_north(class port *A, class port *B)
{
	if (A->get_linked_element()->get_X() < B->get_linked_element()->get_X())
		return true;
	if (A->get_linked_element()->get_X() > B->get_linked_element()->get_X())
		return false;
	if (A->get_linked_element()->get_Y() < B->get_linked_element()->get_Y())
		return true;
	return false;
}
static bool compare_south(class port *A, class port *B)
{
	if (A->get_linked_element()->get_X() < B->get_linked_element()->get_X())
		return true;
	if (A->get_linked_element()->get_X() > B->get_linked_element()->get_X())
		return false;
	if (A->get_linked_element()->get_Y() > B->get_linked_element()->get_Y())
		return true;
	return false;
}
static bool compare_west(class port *A, class port *B)
{
	if (A->get_linked_element()->get_Y() < B->get_linked_element()->get_Y())
		return true;
	if (A->get_linked_element()->get_Y() > B->get_linked_element()->get_Y())
		return false;
	if (A->get_linked_element()->get_X() < B->get_linked_element()->get_X())
		return true;
	return false;
}
static bool compare_east(class port *A, class port *B)
{
	if (A->get_linked_element()->get_Y() < B->get_linked_element()->get_Y())
		return true;
	if (A->get_linked_element()->get_Y() > B->get_linked_element()->get_Y())
		return false;
	if (A->get_linked_element()->get_X() > B->get_linked_element()->get_X())
		return true;
	return false;
}

void model_nest::regen_ports(void)
{
	north.clear();
	south.clear();
	east.clear();
	west.clear();
	
	/* refresh the scene from the canvas -- undo can have replaced it */
	if (canvas)
		_scene = canvas->get_scene();
		
	_scene->update_name(name);
	if (canvas)
		canvas->update_window_title();
		
	bX1 = 60000000;
	bX2 = -1000;
	bY1 = 60000000;
	bY2 = -10000;
	
	for (auto elem : _scene->elements) {
		if (bX1 > elem->get_X())
			bX1 = elem->get_X();
		if (bX2 < elem->get_X())
			bX2 = elem->get_X();
		if (bY1 > elem->get_Y())
			bY1 = elem->get_Y();
		if (bY2 < elem->get_Y())
			bY2 = elem->get_Y();
			
	}
	
	/* slightly enlarge the bounding box -- makes a lot of math easier */
	bX2 += 0.1;
	bY2 += 0.1;
	bX1 -= 0.1;
	bY1 -= 0.1;
	
	/* approach: walk the list of all elements in the scene, find matched uuid pairs
	   from our port list .. if no match, create a new port.
	   Move the found/new port into one of the four wind directions.
	   At the end, what's left in the port list is extra and goes away
	 */
	for (auto elem : _scene->elements) {
		bool found = false;
		if (elem->class_id() != "model_toggle:" && elem->class_id() != "model_output:") {
			continue;
		}
			
		for (unsigned int i = 0; i < ports.size(); i++) {
			class port *_port = ports[i];
			if (elem->get_uuid() == _port->get_linked_uuid()) {
				_port->set_linked_element(elem);
				_port->update_name(elem->get_name());
				place_port(_port);
				ports.erase(ports.begin() + i);
				found = true;
				break;
			}
		}
		if (!found) {
			printf("Adding new port \n");
			class port *_port;
			int direction = PORT_IN;
			if (elem->class_id() == "model_output:")
				direction = PORT_OUT;
			_port = new port(elem->get_name(), direction);
			_port->X = 0;
			_port->Y = 0;;
			_port->parent = this;
			_port->value = {};	
			_port->set_linked_element(elem);
			_port->link_uuid(elem->get_uuid());
			place_port(_port);
		}
		
	}
	
	/* what is left needs to go away */
	while (ports.size() > 0) {
		class port *_port;
		_port = ports[0];
		_port->remove_wires();
		ports.erase(ports.begin());
		delete _port;
	}
	
	sizeX = std::max(west.size() + 2U, std::max(east.size() + 2, 4UL));	
	sizeY = std::max(north.size() + 2U, std::max(south.size() + 2, 4UL));
	
	std::sort(north.begin(), north.end(), compare_north);
	std::sort(south.begin(), south.end(), compare_south);
	std::sort(east.begin(), east.end(), compare_east);
	std::sort(west.begin(), west.end(), compare_west);
	
	/* center */
	unsigned int offset = (sizeY - north.size())/2;
	
	for (unsigned int i = 0; i < north.size(); i++) {
		north[i]->X = i + offset;
		north[i]->Y = -1;
	}
	
	offset = (sizeY - south.size())/2;
	for (unsigned int i = 0; i < south.size(); i++) {
		south[i]->X = i + 1;
		south[i]->Y = sizeX;
	}
	
	offset = (sizeX - west.size())/2;
	for (unsigned int i = 0; i < west.size(); i++) {
		west[i]->X = -1;
		west[i]->Y = i + 1;
	}
	
	offset = (sizeX - east.size())/2;
	for (unsigned int i = 0; i < east.size(); i++) {
		east[i]->X = sizeX;
		east[i]->Y = i + 1;
	}

	
	
	
	ports.insert(ports.end(), north.begin(), north.end());
	ports.insert(ports.end(), south.begin(), south.end());
	ports.insert(ports.end(), east.begin(), east.end());
	ports.insert(ports.end(), west.begin(), west.end());
}


void model_nest::place_port(class port *port)
{
	float rX, rY;
	float dN,dS,dE,dW;
	
	rX = (port->get_linked_element()->get_X() - bX1) / (bX2 - bX1);
	rY = (port->get_linked_element()->get_Y() - bY1) / (bY2 - bY1);
	
	/* first, the easy cases: at or too near the border */
	if (rX < 0.05) {
		west.push_back(port);
		return;
	}
	if (rX > 0.95) {
		east.push_back(port);
		return;
	}
	if (rY < 0.05) {
		north.push_back(port);
		return;
	}
	if (rY > 0.95) {
		south.push_back(port);
		return;
	}

	/* then -- just pick the nearest based on the mid points of the edges of the relative square */
	dW = dist(0,0.5, rX, rY);
	dE = dist(1,0.5, rX, rY);
	dN = dist(0.5,0, rX, rY);
	dS = dist(0.5,1, rX, rY);
	
	if (dW <= dE && dW <= dN && dW <= dS) {
		west.push_back(port);
		return;
	}
	if (dE <= dN && dE <= dS) {
		east.push_back(port);
		return;
	}
	if (dN <= dS) {
		north.push_back(port);
		return;
	}
	south.push_back(port);	
}


void model_nest::load_scene_from_json(std::string logic)
{
     json j = json::parse(logic);
     _scene->from_json(j);
     regen_ports();
}



void model_nest::maybe_regen_ports(void)
{
	if (canvas)
		_scene = canvas->get_scene();

	if (previous_generation_count != _scene->get_generation_count()) {
		regen_ports();
		previous_generation_count = _scene->get_generation_count();
	}
}