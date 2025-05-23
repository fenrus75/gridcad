#include "gridcad.h"
#include "widget.h"


tristate::tristate(float X, float Y, float W, float H, struct truthvalue *_tv, int _gridX, int _gridY) : widget(X,Y,W,H)
{
	tv = _tv;
	gridX = _gridX;
	gridY = _gridY;
}

tristate::~tristate(void)
{
}

void tristate::handle_event(SDL_Event &event)
{
}

void tristate::draw(class basecanvas *canvas)
{
	if (_selected)
	        canvas->drawBox(X1 + 0.1, Y1 + 0.1, X2 - 0.1, Y2- 0.1, COLOR_ELEMENT_ORIGIN);
	else
	        canvas->drawBox(X1 + 0.1, Y1 + 0.1, X2 - 0.1, Y2- 0.1, COLOR_BACKGROUND_MAIN);
	canvas->draw_text(""+tv->Token, X1 + 0.1, Y1 + 0.1, X2 - X1 - 0.2, Y2 - Y1 - 0.2);
}