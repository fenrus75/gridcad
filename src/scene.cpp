#include <stdio.h>

#include "gridcad.h"

#include "iconbar.h"


scene::scene(void)
{
	sizeX = 200;
	sizeY = 200;
}

scene::~scene(void)
{
}



void scene::add_element(class element * element)
{
	elements.push_back(element);
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

