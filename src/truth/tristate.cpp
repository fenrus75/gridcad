#include "gridcad.h"
#include "widget.h"


tristate::tristate(float X, float Y, float W, float H, struct truthvalue *_tv) : widget(X,Y,W,H)
{
	tv = _tv;
}

tristate::~tristate(void)
{
}

void tristate::handle_event(SDL_Event &event)
{
}

void tristate::draw(class basecanvas *canvas)
{
        canvas->drawBox(X1, Y1, X2-X1, Y2-Y1, COLOR_BACKGROUND_MAIN);
	canvas->draw_text(""+tv->Token, X1 + 0.1, Y1 + 0.1, X2 - X1 - 0.2, Y2 - Y1 - 0.2);
}