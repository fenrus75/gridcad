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
#include "canvas.h"
#include "scene.h"
#include "element.h"

#include "model_nest.h"
#include "model_toggle.h"
#include "model_output.h"
#include "model_clock.h"
#include "port.h"
#include "library.h"
#include "contextmenu.h"
#include "name.h"


#include <algorithm>
#include <sys/time.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>

static float dist(float X1, float Y1, float X2, float Y2)
{
	return sqrtf( (X2-X1)*(X2-X1) + (Y2-Y1)*(Y2-Y1));
}

void callback_reset(class element *element)
{
	class model_nest *nest = (class model_nest *) element;
	nest->reset_to_library();
}

void callback_save_to_library(class element *element)
{
	SDL_Event ev = {};


	ev.type = EVENT_SAVE_TO_LIBRARY;
	ev.user.code = 0;
	ev.user.data1 = element;
	SDL_PushEvent(&ev);
}


model_nest::model_nest(float _X, float _Y) : element(_X,_Y, "SubScreen")
{
	sizeX = 4;
	sizeY = 4;
	X = floorf(_X);
	Y = floorf(_Y);    
	//	printf("New scene %s %s\n", name.c_str(), get_full_name().c_str());
	owned_scene = std::make_unique<class scene>(name, get_full_name());
	menu->add_item("Edit name", callback_editname);
	menu->add_item("Reset to library version", callback_reset);
	menu->add_item("Save to library", callback_save_to_library);

	name_edit = std::make_unique<class name>(&name);
}

class scene *model_nest::active_scene(void) const
{
	return canvas ? canvas->get_scene() : owned_scene.get();
}

void model_nest::set_scene(std::unique_ptr<class scene> new_scene)
{
	owned_scene = std::move(new_scene);
	regen_ports();
}

model_nest::~model_nest(void) = default;


void model_nest::drawAt(class basecanvas *canvas, float X, float Y, int type)
{
	maybe_regen_ports();
	if (!selected)
		name_edit->set_edit_mode(false);
	if (!selected) {
		if (icon == "") {
			canvas->draw_image_rotated("assets/nest/nest_back.png", X, Y, sizeX, sizeY, Alpha(type), angle);
			canvas->draw_image_rotated("assets/nest/icon.png", X+0.15, Y+0.15, sizeX-0.3, sizeY-0.3, Alpha(type), angle);	
		} else {
			canvas->draw_image_rotated(icon, X, Y, sizeX, sizeY, Alpha(type), angle);
		}
	} else {
		if (icon_selected == "") 
			canvas->draw_image_rotated("assets/nest/nest_selected.png", X, Y, sizeX, sizeY, Alpha(type), angle);
		else {
			canvas->draw_image_rotated(icon_selected, X, Y, sizeX, sizeY, Alpha(type), angle);
			if (icon == icon_selected)
				canvas->draw_image_rotated("assets/selectionhalo.png", X-0.1, Y-0.1, sizeX+0.2, sizeY+0.2, Alpha(type), angle);
		}
	}

	if (title_on_top)
		name_edit->drawAt(canvas, X, Y -1, sizeX);
	else
		name_edit->drawAt(canvas, X, Y + sizeY, sizeX);

	for (auto &port : ports) {
		port->draw_wires(canvas);	
		port->drawAt(canvas, X, Y, type);
	}

	hover_ports(canvas);     	 

}


void model_nest::calculate(int ttl)
{
	maybe_regen_ports();
	add_nets();
	for (auto &port : ports) {
		if (port->direction == PORT_IN) {
			class model_toggle *tog;
			class element *elmt;
			elmt = port->get_linked_element();
			tog = (class model_toggle *) elmt;

			if (tog && elmt->class_id() == "model_toggle:") {
				tog->update_value(&(port->value), ttl-1);
				port->set_width(tog->get_width());
			}
		}
	}
	for (auto &port : ports) {
		if (port->direction == PORT_OUT) {
			class model_output *tog;
			struct value val = {};
			tog = (class model_output *)port->get_linked_element();
			if (tog) {
				val = tog->get_value();
				port->update_value_net(&val, ttl-1);
				port->update_value_final(&val, ttl-1);
				port->set_width(tog->get_width());
			}
		}
	}
}


bool model_nest::mouse_select(float _X, float _Y)
{
	long int click = time(nullptr);

	regen_ports();

	if (click - previous_click > 1) {
		previous_click = click;
		return false;
	}	

	if (!canvas) {
		logger::get().info("Spawning a new window");
		canvas = std::make_unique<class canvas>(std::move(owned_scene));
		canvas->unhide();
		register_new_canvas(canvas.get());	
	} else {
		canvas->unhide();
	}
	previous_click = click;
	return false;
}


void model_nest::handle_event(class basecanvas *thiscanvas, SDL_Event &event)
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
	active_scene()->to_json(p);
	j["scene"] = p;   
	j["icon"] = path_to_template(icon);
	j["icon_selected"] = path_to_template(icon_selected);
	j["from_library_collection"] = path_to_template(from_library_collection);
	j["from_library_element"] = path_to_template(from_library_element);
}
void model_nest::from_json(json &j)
{
	element::from_json(j);
	active_scene()->from_json(j["scene"]);

	icon = template_to_path(j.value("icon", ""));
	icon_selected = template_to_path(j.value("icon_selected", ""));
	from_library_collection = template_to_path(j.value("from_library_collection", ""));
	from_library_element = template_to_path(j.value("from_library_element", ""));

	if (from_library_collection == "")
		menu->set_inactive("Reset to library version");
	else
		menu->set_active("Reset to library version");
	regen_ports();
}

static bool compare_north(const std::unique_ptr<class port> &A, const std::unique_ptr<class port> &B)
{
	if (A->get_linked_element()->get_X() < B->get_linked_element()->get_X())
		return true;
	if (A->get_linked_element()->get_X() > B->get_linked_element()->get_X())
		return false;
	if (A->get_linked_element()->get_Y() < B->get_linked_element()->get_Y())
		return true;
	return false;
}
static bool compare_south(const std::unique_ptr<class port> &A, const std::unique_ptr<class port> &B)
{
	if (A->get_linked_element()->get_X() < B->get_linked_element()->get_X())
		return true;
	if (A->get_linked_element()->get_X() > B->get_linked_element()->get_X())
		return false;
	if (A->get_linked_element()->get_Y() > B->get_linked_element()->get_Y())
		return true;
	return false;
}
static bool compare_west(const std::unique_ptr<class port> &A, const std::unique_ptr<class port> &B)
{
	if (A->get_linked_element()->get_Y() < B->get_linked_element()->get_Y())
		return true;
	if (A->get_linked_element()->get_Y() > B->get_linked_element()->get_Y())
		return false;
	if (A->get_linked_element()->get_X() < B->get_linked_element()->get_X())
		return true;
	return false;
}
static bool compare_east(const std::unique_ptr<class port> &A, const std::unique_ptr<class port> &B)
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

	title_on_top = false;

	class scene *_scene = active_scene();

	_scene->update_name(name);
	if (canvas)
		canvas->update_window_title();

	bX1 = 60000000;
	bX2 = -1000;
	bY1 = 60000000;
	bY2 = -10000;

	for (auto &elem : _scene->elements) {
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
	for (auto &elem : _scene->elements) {
		bool found = false;
		if (elem->class_id() != "model_toggle:" && elem->class_id() != "model_output:" && elem->class_id() != "model_clock:") {
			continue;
		}

		for (unsigned int i = 0; i < ports.size(); i++) {
			ports[i]->set_linked_element(nullptr);
			if (elem->get_uuid() == ports[i]->get_linked_uuid()) {
				auto _port = std::move(ports[i]);
				ports.erase(ports.begin() + i);
				_port->set_linked_element(elem.get());
				_port->update_name(elem->get_name());
				place_port(std::move(_port));
				found = true;
				break;
			}
		}
		if (!found) {
			logger::get().debug("Adding new port");
			int direction = PORT_IN;
			if (elem->class_id() == "model_output:")
				direction = PORT_OUT;
			auto _port = std::make_unique<port>(elem->get_name(), direction);
			_port->X = 0;
			_port->Y = 0;;
			_port->parent = this;
			_port->value = {};	
			_port->set_linked_element(elem.get());
			_port->link_uuid(elem->get_uuid());
			place_port(std::move(_port));
		}

	}

	/* what is left needs to go away */
	for (auto &_port : ports) {
		_port->remove_wires();
	}
	ports.clear();

	sizeY = std::max(west.size() + 2U, std::max(east.size() + 2, 4UL));	
	sizeX = std::max(north.size() + 2U, std::max(south.size() + 2, 4UL));

	std::sort(north.begin(), north.end(), compare_north);
	std::sort(south.begin(), south.end(), compare_south);
	std::sort(east.begin(), east.end(), compare_east);
	std::sort(west.begin(), west.end(), compare_west);

	if (north.size() == 0 && south.size() > 0)
		title_on_top = true;

	/* center */
	unsigned int offset = (sizeX - north.size())/2;

	for (unsigned int i = 0; i < north.size(); i++) {
		north[i]->X = i + offset;
		north[i]->Y = -1;
	}

	offset = (sizeX - south.size())/2;
	for (unsigned int i = 0; i < south.size(); i++) {
		south[i]->X = i + offset;
		south[i]->Y = sizeY;
	}

	offset = (sizeY - west.size())/2;
	for (unsigned int i = 0; i < west.size(); i++) {
		west[i]->X = -1;
		west[i]->Y = i + offset;
	}

	offset = (sizeY - east.size())/2;
	for (unsigned int i = 0; i < east.size(); i++) {
		east[i]->X = sizeX;
		east[i]->Y = i + offset;
	}




	for (auto &p : north)
		ports.push_back(std::move(p));
	for (auto &p : south)
		ports.push_back(std::move(p));
	for (auto &p : east)
		ports.push_back(std::move(p));
	for (auto &p : west)
		ports.push_back(std::move(p));
	north.clear();
	south.clear();
	east.clear();
	west.clear();
	remove_nets();
	add_nets();
}


void model_nest::place_port(std::unique_ptr<class port> port)
{
	float rX, rY;
	float dN,dS,dE,dW;

	rX = (port->get_linked_element()->get_X() - bX1) / (bX2 - bX1);
	rY = (port->get_linked_element()->get_Y() - bY1) / (bY2 - bY1);

	/* first, the easy cases: at or too near the border */
	if (rX < 0.05) {
		west.push_back(std::move(port));
		return;
	}
	if (rX > 0.95) {
		east.push_back(std::move(port));
		return;
	}
	if (rY < 0.05) {
		north.push_back(std::move(port));
		return;
	}
	if (rY > 0.95) {
		south.push_back(std::move(port));
		return;
	}

	/* then -- just pick the nearest based on the mid points of the edges of the relative square */
	dW = dist(0,0.5, rX, rY);
	dE = dist(1,0.5, rX, rY);
	dN = dist(0.5,0, rX, rY);
	dS = dist(0.5,1, rX, rY);

	if (dW <= dE && dW <= dN && dW <= dS) {
		west.push_back(std::move(port));
		return;
	}
	if (dE <= dN && dE <= dS) {
		east.push_back(std::move(port));
		return;
	}
	if (dN <= dS) {
		north.push_back(std::move(port));
		return;
	}
	south.push_back(std::move(port));
}


void model_nest::load_scene_from_json(std::string_view logic)
{
	json j = json::parse(logic);
	active_scene()->from_json(j);
	regen_ports();
}



void model_nest::maybe_regen_ports(void)
{
	class scene *_scene = active_scene();

	if (previous_generation_count != _scene->get_generation_count()) {
		regen_ports();
		previous_generation_count = _scene->get_generation_count();
	}
}

std::string model_nest::get_verilog_main(void)
{
	std::string s = "";
	class scene *_scene = active_scene();

	_scene->create_verilog_names();

	s = s + _scene->get_verilog_name() + " " + get_verilog_name() + "(";
	bool first = true;
	for (auto &port : ports) {

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

std::string model_nest::get_verilog_modules(std::string path)
{
	std::string s = "";
	class scene *_scene = active_scene();

	s += _scene->get_verilog_main();
	s += _scene->get_verilog_modules(path);

	return s;
}

void model_nest::update_parental_name(std::string _name)
{
	parental_name = _name;
	active_scene()->set_parental_name(get_full_name());
}

void model_nest::create_verilog_name(int seqno, std::vector<std::string> *existing)
{
	element::create_verilog_name(seqno, existing);
	active_scene()->update_vname(verilog_name);

}

void model_nest::reset_to_library(void)
{
	struct library_block *lib;

	if (from_library_collection == "")
		return;

	lib = find_in_library(from_library_collection, from_library_element);
	if (!lib)
		return;

	auto newscene = std::make_unique<scene>(active_scene()->get_name(), get_full_name());
	if (canvas) {
		canvas->swap_scene(std::move(newscene));
	} else {
		owned_scene = std::move(newscene);
	}


	load_scene_from_json(lib->logic);
	set_icon(lib->icon, lib->icon_selected);		
}

extern std::map<std::string, const unsigned char *> datamap;
extern std::map<std::string, unsigned int> sizemap;

void model_nest::save_to_library(std::string library_path)
{
	std::string icon;
	logger::get().info("Library path is {}", library_path);

	if (library_path != "")
		std::filesystem::create_directory(library_path);

	json j;

	active_scene()->to_json(j);

	std::string outputfile = "";
	if (library_path != "")
		outputfile = library_path + "/" + name;
	else
		outputfile = name;

	if (!outputfile.ends_with(".json"))
		outputfile = outputfile +  ".json";

	logger::get().info("Outputfile is {}", outputfile);

	std::ofstream output(outputfile);

	output << j.dump(4);
	output.close();
	if (!std::filesystem::exists(library_path + "/" + name + ".json.png")) {
		/* write an icon */
		const unsigned char *pixels = nullptr;
		unsigned int size;
		srand(time(nullptr));
		while (pixels == nullptr) {
			int i = rand() % 64;
			icon = std::format("assets/random/icon{}.png", i);
			if (datamap.find(icon) == datamap.end())
				continue;
			pixels = datamap[icon];
			size = sizemap[icon];
		}

		std::ofstream outputpng(library_path + "/" + name + ".json.png",  std::ios::binary);

		outputpng.write((const char *)pixels, size);
		outputpng.close();
	}

	/* write a tooltip */	
	if (!std::filesystem::exists(library_path + "/" + name + ".json.tooltip")) {
		std::ofstream outputtt(library_path + "/" + name + ".json.tooltip",  std::ios::binary);

		outputtt << "Custom library element for a '" + name + "'";
		outputtt.close();
	}
	if (!std::filesystem::exists(library_path + "/" + name + ".json.overlay")) {

		std::ofstream outputol(library_path + "/" + name + ".json.overlay",  std::ios::binary);

		outputol << name;
		outputol.close();
	}
}
