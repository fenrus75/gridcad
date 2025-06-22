#include "gridcad.h"
#include "contextmenu.h"

#include <algorithm>


static float scale = -1;


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
    item->active = true;
    
    items.push_back(item);
}

void contextmenu::add_item(std::string text, scene_callback_fn scene_callback)
{
    struct context_item *item;
    
    item = new struct context_item;
    
    item->menu_text = text;
    item->scene_callback = scene_callback;
    item->active = true;
    
    items.push_back(item);
}

void contextmenu::set_active(std::string name)
{
	for (auto item : items) {
		if (item->menu_text == name)
			item->active = true;
	}
}
void contextmenu::set_inactive(std::string name)
{
	for (auto item : items) {
		if (item->menu_text == name)
			item->active = false;
	}
}

float Xsize(class basecanvas *canvas, std::string str)
{
	SDL_Texture *text;
	
	text = canvas->text_to_texture(str)	;
	if (!text)
		return 0;
		
	SDL_Point size = {};
	
	SDL_QueryTexture(text, NULL, NULL, &size.x, &size.y);
	
	return size.x;
}

float Ysize(class basecanvas *canvas, std::string str)
{
	SDL_Texture *text;

	text = canvas->text_to_texture(str)	;
	if (!text)
		return 0;
		
	SDL_Point size = {};
	
	SDL_QueryTexture(text, NULL, NULL, &size.x, &size.y);
	
	return size.y;
}


void draw_menu_item(class basecanvas *canvas, float X, float Y, float W, float H, std::string string, bool selected)
{
	SDL_Texture *shade;
	
	if (selected)
		shade = canvas->load_image("assets/lightgray.png");
	else
		shade = canvas->load_image("assets/gray.png");
		
	canvas->draw_image(shade, X, Y, W, H);
	canvas->draw_text_left(string, X, Y, W, H);
}

void draw_menu_item_color(class basecanvas *canvas, float X, float Y, float W, float H, std::string string, bool selected, int color)
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
	canvas->draw_text_left(string, X + canvas->scale_to_X(36), Y, W, H);
}


static void fill_scale(void)
{
	if (scale > 0)
		return;
    	float dpi;
    	SDL_GetDisplayDPI(0, &dpi, NULL, NULL);
    	if (dpi < 90)
    		dpi = 96;
    	scale = 1.3 * 96.0 / dpi;
}

float get_scale(void)
{
	fill_scale();
	return scale;
}


void contextmenu::draw_at(class basecanvas *canvas, float X, float Y)
{

    fill_scale();

    maxX = 0;
    maxY = 0;

    for (auto item : items) {
        maxX = std::max(maxX, Xsize(canvas, item->menu_text));
        maxY = std::max(maxY, Ysize(canvas, item->menu_text));
    }
    
    maxX = maxX / scale;
    maxY = maxY / scale;
    maxX = canvas->scale_to_X(maxX);
    maxY = canvas->scale_to_Y(maxY);
    
    X1 = X;
    Y1 = Y;
    
    for (unsigned int i = 0; i < items.size(); i++) {
    	auto item = items[i];
    	std::string asterix = "";
    	if (!item->active)
    		asterix = "*";
    	draw_menu_item(canvas, X, Y, maxX, maxY, asterix + item->menu_text, ((int)i == selection) && item->active);
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
	if (selection < (int)items.size() && element && items[selection]->callback)
		items[selection]->callback(element);
	if (selection < (int)items.size() && scene && items[selection]->scene_callback)
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
    item->active = true;
    
    items.push_back(item);
}

void port_contextmenu::draw_at(class basecanvas *canvas, float X, float Y)
{
    fill_scale();
    
    maxX = 0;
    maxY = 0;

    for (auto item : items) {
        maxX = std::max(maxX, Xsize(canvas, item->menu_text));
        maxY = std::max(maxY, Ysize(canvas, item->menu_text));
    }
    
    maxX = maxX / scale;
    maxY = maxY / scale;
    maxX = canvas->scale_to_X(maxX);
    maxY = canvas->scale_to_Y(maxY);
    
    X1 = X;
    Y1 = Y;
    
    for (unsigned int i = 0; i < items.size(); i++) {
    	auto item = items[i];
    	draw_menu_item_color(canvas, X, Y, maxX, maxY, item->menu_text, (int)i == selection, item->color);
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


icon_contextmenu::icon_contextmenu(class iconbar *_bar) : contextmenu( (class element*) NULL)
{
	bar = _bar;
	
}

icon_contextmenu::~icon_contextmenu(void)
{
}


void icon_contextmenu::add_item(std::string text, class oneiconbar* bar, icon_callback_fn callback)
{
    struct context_item *item;
    
    item = new struct context_item;
    
    item->menu_text = text;
    item->icon_callback = callback;
    item->choice = bar;
    item->active = true;
    
    items.push_back(item);
}

void icon_contextmenu::mouse_click(float X, float Y)
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
	if (selection < (int)items.size())
		items[selection]->icon_callback(bar, items[selection]->choice);

}


void icon_contextmenu::draw_at(class basecanvas *canvas, float X, float Y)
{
	contextmenu::draw_at(canvas, X, Y);
}

void icon_contextmenu::draw_at(class basecanvas *canvas)
{
	float sX, sY, sW;
	sX = currentX;
	sY = currentY;
	
	sW = canvas->screen_width();

	
	if (sX + maxX > sW)
		sX = sW - maxX;

	draw_at(canvas, sX, sY);

}
