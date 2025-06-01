#include "gridcad.h"
#include "contextmenu.h"

#include <algorithm>


contextmenu::contextmenu(class element *_element)
{
    element = _element;
}

contextmenu::~contextmenu(void)
{
}

void contextmenu::add_item(std::string text, callback_fn callback)
{
    struct context_item *item;
    
    item = new struct context_item;
    
    item->menu_text = text;
    item->callback = callback;
    
    
    
    
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
	
	return canvas->scr_to_X(size.x/scale);
}

float Ysize(class basecanvas *canvas, std::string str, float scale)
{
	SDL_Texture *text;
	
	text = canvas->text_to_texture(str)	;
	if (!text)
		return 0;
		
	SDL_Point size = {};
	
	SDL_QueryTexture(text, NULL, NULL, &size.x, &size.y);
	
	printf("size.y is %i \n", size.y);
	return canvas->scr_to_Y(size.y/scale);
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
	canvas->draw_text(string, X, Y, W, 1);
}


void contextmenu::draw_at(class basecanvas *canvas, float X, float Y)
{
    const float scale = 1.5;

    maxX = 0;
    maxY = 0;

#if 0
    for (auto item : items) {
        maxX = std::max(maxX, Xsize(canvas, item->menu_text, scale));
        maxY = std::max(maxY, Ysize(canvas, item->menu_text, scale));
	    printf("MaxY is %5.2f\n", maxY);
	    printf("MaxX is %5.2f\n", maxX);
    }
    #endif
    maxY = 1;
    maxX = 3.5;
    
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
	draw_at(canvas, element->get_X(), element->get_Y() + element->get_height()/2);
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

void contextmenu::mouse_click(float X, float Y)
{
	selection = -1;
	
	if (X < X1 || X > X2)
		return;
	if (Y < Y1 || Y > Y2)
		return;
		
	Y -= Y1;
	selection = floorf(Y/maxY);
	if (selection < (int)items.size())
		items[selection]->callback(element);
}
