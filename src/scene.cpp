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


scene::scene(std::string _name, std::string _parent)
{
	sizeX = 200;
	sizeY = 200;
	name = name;
	parental_name = _parent;
}

scene::~scene(void)
{
	for (auto elem: elements) {
		delete elem;
	}
}



void scene::add_element(class element * element)
{
	elements.push_back(element);
	rewire_section(element->get_X(), element->get_Y(), element->get_width(), element->get_height());
        generation_count++;
	element->update_parental_name(get_full_name());
}

bool scene::can_place_element(float x, float y, int w, int h,
			      class element * myself)
{
	for (auto const elem:elements) {
		int _x, _y;
		if (elem == myself)
			continue;
		for (_y = -1; _y <= h + 1; _y++)
			for (_x = -1; _x <= w + 1 ; _x++)
				if (elem->intersect(x + _x, y + _y))
					return false;
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
	}
	for (auto element : elements)
		element->calculate(100);
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

