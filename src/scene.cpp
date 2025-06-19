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

#include <stdio.h>

#include "gridcad.h"
#include "wire.h"
#include "contextmenu.h"

void callback_select_all(class scene *scene)
{
	for (auto elem : scene->elements) {
		elem->select();
	}
}

void callback_fit_to_screen(class scene *scene)
{
	SDL_Event ev = {};


	ev.type = EVENT_ZOOM_TO_FIT;
	ev.user.code = 0;
	ev.user.data1 = scene;
	
	SDL_PushEvent(&ev);
}

void callback_autoclock(class scene *scene)
{
	class port *clk = NULL;
	for (auto elem : scene->elements) {
		clk = elem->get_clk_port();
		if (clk) 
			break;
	}
	if (!clk)
		return;
	for (auto elem : scene->elements) {
		elem->connect_clk(clk);
	}
}

void callback_reroute(class scene *scene)
{
	scene->reroute_all_wires();
}

scene::scene(std::string _name, std::string _parent)
{
	sizeX = 200;
	sizeY = 200;
	name = _name;
	parental_name = _parent;
	menu = new class contextmenu(this);
	menu->add_item("Fit to Screen", callback_fit_to_screen);
	menu->add_item("Select All", callback_select_all);
	menu->add_item("Connect clocks", callback_autoclock);
	menu->add_item("Reroute wires", callback_reroute);
}

scene::~scene(void)
{
	for (auto elem: elements) {
		remove_from_calculate_queue(elem); 
		delete elem;
	}
	delete menu;
}



void scene::add_element(class element * element)
{
	elements.push_back(element);
	rewire_section(element->get_X(), element->get_Y(), element->get_width(), element->get_height());
        generation_count++;
	element->update_parental_name(get_full_name());
	printf("Element parnetal name is %s   name is %s parent is %s \n", element->get_parental_name().c_str(), name.c_str(), parental_name.c_str());
}

bool scene::can_place_element(float x, float y, int w, int h,
			      class element * myself)
{
	for (auto const elem:elements) {
		int _x, _y;
		if (elem == myself)
			continue;
		if (elem->is_background()) /* labels are special and can be placed upon */
			continue;
		for (_y = -2; _y < h + 2; _y++)
			for (_x = -2; _x < w + 2 ; _x++)
				if (myself->intersect_float(x + _x, y + _y) && elem->intersect_full(x + _x, y + _y)) {
					return false;
				}
	}
	return true;
}

class port *scene::is_port(float X, float Y)
{
	for (auto const elem:elements) {
		class port *p = elem->is_port(X, Y);
		if (p)
			return p;
	}
	return NULL;

}

class wire *scene::is_wire(float X, float Y)
{
	for (auto const elem:elements) {
		class wire *p = elem->is_wire(X, Y);
		if (p) {
			return p;
		}
	}
	return NULL;
}

void scene::fill_grid(class wiregrid * grid)
{
	for (auto const elem:elements)
		elem->fill_grid(grid);
}

void scene::deselect_all(void)
{
	for (auto element : elements)
		element->deselect();
}
class element * scene::selected_element(void)
{
	for (auto element : elements)
		if (element->is_selected())
			return element;
	return NULL;
}

void scene::remove_element(class element *element)
{
	unsigned int i;
	for (i = 0; i < elements.size(); i++) {
		if (elements[i] == element) {
			elements.erase(elements.begin() + i); /* must exit the for loop now */
			remove_from_calculate_queue(element);
			delete element;
			return;
		}
	}
        generation_count++;
}

void scene::to_json(json &j)
{
	unsigned int i;
	j["sizeX"] = sizeX;
	j["sizeY"] = sizeY;
	j["elements"] = json::array();
	j["name"] = name;
	j["parental_name"] = parental_name;
	j["verilog_name"] = verilog_name;
	
	for (i = 0; i < elements.size(); i++) {
		json p;
		elements[i]->to_json(p);
		j["elements"][i] = p;
	}
}

void scene::selection_to_json(json &j)
{
	unsigned int i;
	unsigned int q = 0;
	j["elements"] = json::array();
	
	name = j.value("name", "");
	parental_name = j.value("parental_name", "");
	verilog_name = j.value("verilog_name", "");
	
	for (i = 0; i < elements.size(); i++) {
		if (elements[i]->is_selected()) {
			json p;
			elements[i]->to_json(p);
			j["elements"][q++] = p;
		}
	}
}

void scene::from_json(json &j)
{
	unsigned int i;

	sizeX = j["sizeX"];
	sizeY = j["sizeY"];
	for (i = 0; i <j["elements"].size(); i++) {
		json p = j["elements"][i];
		std::string cid = p["class_id"];
		class element *element = element_from_class_id(cid);
		element->from_json(p);
		elements.push_back(element);
		element->update_parental_name(get_full_name());
	}
	for (auto element : elements)
		element->calculate(100);

	cycle_color();
	redo_nets();
}

void scene::process_delete_requests(void)
{
	std::vector<class element *> todo;
	
	for (auto elem : elements) {
		if (elem->want_deleted())
			todo.push_back(elem);
	}
	
	for (auto elem : todo) {
		remove_element(elem);
	        generation_count++;
	}
}

void scene::delete_selection(void)
{
	for (auto elem : elements)
	        if (!elem->in_edit_mode())
			elem->delete_if_selected();
}


void scene::rewire_section(int x1, int y1, int w, int h)
{
	int x, y;

	for (y = y1 - 1 ; y <= y1 + h + 1; y++)
		for (x = x1 -1 ; x <= x1 + w + 1; x++) {
			class wire *wire;
			wire = is_wire(x, y);
			if (wire) {
				wire->reseat();
				wire->route(this);
			}
		}
}

void scene::rewire_section(class element *element)
{
	rewire_section(element->get_X(), element->get_Y(), element->get_width(), element->get_height());
}

void scene::remove_orphans(void)
{
	for (auto elem : elements)
		elem->remove_orphans();
}

unsigned int scene::selected_count(void)
{
	unsigned int count = 0;
	for (auto elem : elements)
		if (elem->is_selected()) 
			count++;
			
	return count;
}


void scene::cycle_color(void)
{
	for (auto elem:elements)
		elem->cycle_color();
	redo_nets();
}

void scene::reroute_all_wires(void)
{
	for (auto elem:elements)
		elem->reroute_all_wires();
}

void scene::create_verilog_names(void)
{
	if (verilog_name == "") {
		verilog_name = parental_name + "_" + name;
		if (vname != "")
			verilog_name = parental_name + "_" + vname;
		verilog_name = append_random_bits(verilog_name + "_");
		
		if (parental_name == "" && name == "main")
			verilog_name = "main";
	}
	std::replace(verilog_name.begin(), verilog_name.end(), '-', '_');
	std::replace(verilog_name.begin(), verilog_name.end(), '+', '_');
	std::replace(verilog_name.begin(), verilog_name.end(), '/', '_');
	
	std::vector<std::string> existing;
	for (unsigned int i = 0; i < elements.size(); i++) {
		auto elem = elements[i];
		elem->create_verilog_name(i, &existing);
	}
}

std::string scene::get_verilog_main(void)
{
	std::string s = "";
	
	redo_nets();
	
	if (name == "")
		name = "main_scene";
	
	create_verilog_names();
	
	s = "module " + verilog_name + "\n";
	
	s += "(";
	/* in */
	bool first = true;
	for (auto elem : elements) {
		if (elem->class_id() == "model_toggle:") {
			if (!first)
				s += ", ";
			first = false;
			s += "input " + elem->get_verilog_width() + " " +elem->get_verilog_name();
		}
		if (elem->class_id() == "model_clock:") {
			if (!first)
				s += ", ";
			first = false;
			s += "input " + elem->get_verilog_width() + " " +elem->get_verilog_name();
		}
		if (elem->class_id() == "model_output:") {
			if (!first)
				s += ", ";
			first = false;
			s += "output " + elem->get_verilog_width() + " " +elem->get_verilog_name();
		}
	}	
	s += ");\n\n";
	
	
	std::vector<std::string> netmap;
	
	for (auto elem : elements) {
		elem->collect_nets(&netmap);
	}
	
	for (auto netstr : netmap)
		s = s + netstr + "\n";
	
	s = s + "\n";
	
	for (auto elem : elements) {
		s = s + elem->get_verilog_main();
	}	
	
	
	
	s += "endmodule\n\n";
	

	
	return s;
}


std::string scene::get_verilog_modules(std::string verilog_path)
{
	std::string s = "";
	
	for (auto elem : elements) {
		s = s + elem->get_verilog_modules(verilog_path);
	}	
	
	return s;
}
std::string scene::get_full_name(void) 
{ 
	if (vname != "")
		return parental_name + "/" + vname;
	return parental_name + "/" + name;
};


void scene::remove_nets(void)
{
	for (auto elem : elements)
		elem->remove_nets();
}

void scene::add_nets(void)
{
	for (auto elem : elements)
		elem->add_nets();
}

void scene::redo_nets(void)
{
	remove_nets();
	add_nets();
}