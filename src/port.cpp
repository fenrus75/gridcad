#include "gridcad.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_render.h>

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



void port::drawAt(class scene * scene, float _X, float _Y, int type)
{
	SDL_Color white = {255, 255, 255, 255};


	if (!label) {
		SDL_Surface *surface;
		TTF_Font *font;
		font = TTF_OpenFont("fonts/Roboto-Medium-webfont.ttf", 14);
		surface = TTF_RenderUTF8_Blended(font, name, white);
		label = SDL_CreateTextureFromSurface(scene->renderer, surface);
		SDL_FreeSurface(surface);
	}

	if (direction == PORT_IN) {
		drawConnector(scene, _X, _Y, X, Y, COLOR_ELEMENT_CONNECTOR + type);
	} else { 
		drawConnector(scene, _X, _Y, X, Y, value_color(&value));
	}
	draw_wires(scene);
}

void port::draw_wires(class scene * scene)
{
	for (auto wire : wires) {
        	wire->draw(scene);
	}
}

void port::draw(class scene *scene, int color)
{
	drawAt(scene, screenX, screenY, color);
}


void port::drawConnector(class scene * scene, float X, float Y, int cX, int cY, int type)
{
	scene->drawCircle(cX + X + 0.5, cY + Y + 0.5, 0.51, type);
	if (label) {
		double w,h;
		SDL_Point size;
		SDL_QueryTexture(label, NULL, NULL, &size.x, &size.y);
		
		h = 0.3;
		w = size.x / size.y * h;
		
		scene->draw_image(label, cX + X + (1 - w)/2, cY + Y + 1, w, h); 
	}
}

void port::stop_drag(class scene *scene)
{
    for (auto wire : wires) {
        wire->reseat();
        wire->route(scene);
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