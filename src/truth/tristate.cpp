#include "gridcad.h"
#include "widget.h"


tristate::tristate(float X, float Y, float W, float H, struct truthvalue *_tv, int _gridX, int _gridY, bool _is_input) : widget(X,Y,W,H)
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
			tv->Token = '0';
			break;	
		case SDLK_1:
			tv->Token = '1';
			break;	
		case SDLK_x:
			tv->Token = 'X';
			break;	
		}
	break;
	}
}

void tristate::draw(class basecanvas *canvas)
{

	if (tt_one == NULL) {
		if (is_input) {
			tt_one = canvas->load_image("assets/model_truth/tt_in_one.png");
			tt_zero = canvas->load_image("assets/model_truth/tt_in_zero.png");
			tt_dc = canvas->load_image("assets/model_truth/tt_in_dc.png");
		} else {
			tt_one = canvas->load_image("assets/model_truth/tt_out_one.png");
			tt_zero = canvas->load_image("assets/model_truth/tt_out_zero.png");
			tt_dc = canvas->load_image("assets/model_truth/tt_out_dc.png");
		}
	}
	if (_selected)
	        canvas->drawBox(X1 + 0.1, Y1 + 0.1, X2 - 0.1, Y2- 0.1, COLOR_ELEMENT_ORIGIN);
	else
	        canvas->drawBox(X1 + 0.1, Y1 + 0.1, X2 - 0.1, Y2- 0.1, COLOR_BACKGROUND_MAIN);
	        
	if (tv->Token == '0')
		canvas->draw_image(tt_zero, X1,Y1, X2-X1, Y2-Y1, 255, true);
	else if (tv->Token == '1')
		canvas->draw_image(tt_one, X1,Y1, X2-X1, Y2-Y1, 255, true);
	else
		canvas->draw_image(tt_dc, X1,Y1, X2-X1, Y2-Y1, 255, true);
}