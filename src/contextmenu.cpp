#include "gridcad.h"
#include "contextmenu.h"

#include <algorithm>


contextmenu::contextmenu(class element *_element)
{
    element = _element;
}

contextmenu::contextmenu(class scene *_scene)
{
    scene = _scene;
}

contextmenu::~contextmenu(void)
{
	for (auto item : items) {
		delete item;
	}
	items.clear();
}

void contextmenu::add_item(std::string text, callback_fn callback)
{
    struct context_item *item;
    
    item = new struct context_item;
    
    item->menu_text = text;
    item->callback = callback;
    
    items.push_back(item);
}

void contextmenu::add_item(std::string text, scene_callback_fn scene_callback)
{
    struct context_item *item;
    
    item = new struct context_item;
    
    item->menu_text = text;
    item->scene_callback = scene_callback;
    
    items.push_back(item);
}

float Xsize(class basecanvas *canvas, std::string str, float scale)
{
	SDL_Texture *text;
	
	text = canvas->text_to_texture(str)	;
	if (!text)
		return 0;
		
	SDL_Point size = {};
	
	SDL_QueryTexture(text, NULL, NULL, &size.x, &size.y);
	
	return size.x;
}

float Ysize(class basecanvas *canvas, std::string str, float scale)
{
	SDL_Texture *text;
	
	text = canvas->text_to_texture(str)	;
	if (!text)
		return 0;
		
	SDL_Point size = {};
	
	SDL_QueryTexture(text, NULL, NULL, &size.x, &size.y);
	
	return size.y;
}

void draw_menu_item(class basecanvas *canvas, float X, float Y, float W, float H, std::string string, float scale, bool selected)
{
	SDL_Texture *text, *shade;
	
	text = canvas->text_to_texture(string)	;
	if (selected)
		shade = canvas->load_image("assets/lightgray.png");
	else
		shade = canvas->load_image("assets/gray.png");
	if (!text)
		return;
		
	SDL_Point size;
	
	SDL_QueryTexture(text, NULL, NULL, &size.x, &size.y);

	canvas->draw_image(shade, X, Y, W, H);
	canvas->draw_text(string, X, Y, W, H);
}

void draw_menu_item_color(class basecanvas *canvas, float X, float Y, float W, float H, std::string string, float scale, bool selected, int color)
{
	SDL_Texture *text, *shade;
	
	text = canvas->text_to_texture(string)	;
	if (selected)
		shade = canvas->load_image("assets/lightgray.png");
	else
		shade = canvas->load_image("assets/gray.png");
	if (!text)
		return;
		
	SDL_Point size;
	
	SDL_QueryTexture(text, NULL, NULL, &size.x, &size.y);

	canvas->draw_image(shade, X, Y, W + canvas->scale_to_X(36), H);
	canvas->draw_box(X,Y, X + canvas->scale_to_X(36), Y + H, wire_to_color(color));
	canvas->draw_text(string, X + canvas->scale_to_X(36), Y, W, H);
}


void contextmenu::draw_at(class basecanvas *canvas, float X, float Y)
{
    const float scale = 1.5;

    maxX = 0;
    maxY = 0;

    for (auto item : items) {
        maxX = std::max(maxX, Xsize(canvas, item->menu_text, scale));
        maxY = std::max(maxY, Ysize(canvas, item->menu_text, scale));
    }
    
    maxX = maxX / scale;
    maxY = maxY / scale;
    maxX = canvas->scale_to_X(maxX);
    maxY = canvas->scale_to_Y(maxY);
    
    X1 = X;
    Y1 = Y;
    
    for (unsigned int i = 0; i < items.size(); i++) {
    	auto item = items[i];
    	draw_menu_item(canvas, X, Y, maxX, maxY, item->menu_text, scale, (int)i == selection);
    	Y += maxY;
    }
    
    Y2 = Y;
    X2 = X + maxX;
}

void contextmenu::draw_at(class basecanvas *canvas)
{
	draw_at(canvas, currentX, currentY);

}

bool contextmenu::mouse_in_bounds(float X, float Y)
{
	if (X < X1 - 3 || X > X2 + 3)
		return false;
	if (Y < Y1 - 3 || Y > Y2 + 3)
		return false;
	return true;	
}

void contextmenu::mouse_motion(float X, float Y)
{
	selection = -1;

	if (X < X1 || X > X2)
		return;
	if (Y < Y1 || Y > Y2)
		return;
		
	Y -= Y1;
	selection = floorf(Y/maxY);
}

void contextmenu::mouse_set(float X, float Y)
{
	selection = -1;

	currentX = X;
	currentY = Y;
	
}

void contextmenu::mouse_click(float X, float Y)
{
	selection = -1;

	currentX = X;
	currentY = Y;
	
	
	if (X < X1 || X > X2)
		return;
	if (Y < Y1 || Y > Y2)
		return;
		
	Y -= Y1;
	selection = floorf(Y/maxY);
	if (selection < (int)items.size() && element)
		items[selection]->callback(element);
	if (selection < (int)items.size() && scene)
		items[selection]->scene_callback(scene);
}


port_contextmenu:: port_contextmenu(class port *_port) : contextmenu((class element *)NULL)
{
	port = _port;
}

port_contextmenu::~port_contextmenu(void)
{
	for (auto item : items) {
		delete item;
	}
	items.clear();
}

void port_contextmenu::add_item(std::string text, int color, port_callback_fn port_callback)
{
    struct context_item *item;
    
    item = new struct context_item;
    
    item->menu_text = " " + text  + "  ";
    item->port_callback = port_callback;
    item->color = color;
    
    items.push_back(item);
}

void port_contextmenu::draw_at(class basecanvas *canvas, float X, float Y)
{
    const float scale = 1.5;

    maxX = 0;
    maxY = 0;

    for (auto item : items) {
        maxX = std::max(maxX, Xsize(canvas, item->menu_text, scale));
        maxY = std::max(maxY, Ysize(canvas, item->menu_text, scale));
    }
    
    maxX = maxX / scale;
    maxY = maxY / scale;
    maxX = canvas->scale_to_X(maxX);
    maxY = canvas->scale_to_Y(maxY);
    
    X1 = X;
    Y1 = Y;
    
    for (unsigned int i = 0; i < items.size(); i++) {
    	auto item = items[i];
    	draw_menu_item_color(canvas, X, Y, maxX, maxY, item->menu_text, scale, (int)i == selection, item->color);
    	Y += maxY;
    }
    
    Y2 = Y;
    X2 = X + maxX;
}

void port_contextmenu::mouse_click(float X, float Y)
{
	selection = -1;

	currentX = X;
	currentY = Y;
	
	
	if (X < X1 || X > X2)
		return;
	if (Y < Y1 || Y > Y2)
		return;
		
	Y -= Y1;
	selection = floorf(Y/maxY);
	if (selection < (int)items.size() && port)
		items[selection]->port_callback(port, items[selection]->color);
}
