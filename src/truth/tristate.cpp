#include "gridcad.h"
#include "widget.h"


tristate::tristate(float X, float Y, float W, float H, char *_tv, int _gridX, int _gridY, bool _is_input) : widget(X,Y,W,H)
{
	tv = _tv;
	gridX = _gridX;
	gridY = _gridY;
	is_input = _is_input;
}

tristate::~tristate(void)
{
}

void tristate::handle_event(SDL_Event &event)
{
	if (!selected())
		return;
	switch (event.type) {
	case SDL_KEYDOWN:
        	switch (event.key.keysym.sym) {
		case SDLK_0:
			*tv = '0';
			break;	
		case SDLK_1:
			*tv = '1';
			break;	
		case SDLK_x:
			*tv = 'X';
			break;	
		}
	break;
	}
}

void tristate::draw(class basecanvas *canvas)
{

	if (_selected)
	        canvas->drawBox(X1 + 0.1, Y1 + 0.1, X2 - 0.1, Y2- 0.1, COLOR_ELEMENT_ORIGIN);
	else
	        canvas->drawBox(X1 + 0.1, Y1 + 0.1, X2 - 0.1, Y2- 0.1, COLOR_BACKGROUND_MAIN);
	        
	if (is_input) {	        
		if (!zero)
			zero = canvas->load_image("assets/model_truth/tt_in_zero.png");
		if (!one)
			one = canvas->load_image("assets/model_truth/tt_in_one.png");
		if (!dc)
			dc = canvas->load_image("assets/model_truth/tt_in_dc.png");
		if (*tv == '0')
			canvas->draw_image(zero, X1,Y1, X2-X1, Y2-Y1, 255, true);
		else if (*tv == '1')
			canvas->draw_image(one, X1,Y1, X2-X1, Y2-Y1, 255, true);
		else
			canvas->draw_image(dc, X1,Y1, X2-X1, Y2-Y1, 255, true);
	} else {
		if (!zero)
			zero = canvas->load_image("assets/model_truth/tt_out_zero.png");
		if (!one)
			one = canvas->load_image("assets/model_truth/tt_out_one.png");
		if (!dc)
			dc = canvas->load_image("assets/model_truth/tt_out_dc.png");
		if (*tv == '0')
			canvas->draw_image(zero, X1,Y1, X2-X1, Y2-Y1, 255, true);
		else if (*tv == '1')
			canvas->draw_image(one, X1,Y1, X2-X1, Y2-Y1, 255, true);
		else
			canvas->draw_image(dc, X1,Y1, X2-X1, Y2-Y1, 255, true);
	}
}