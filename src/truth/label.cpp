#include "gridcad.h"

#include "widget.h"

label::label(float X, float Y, float W, float H, std::string _text) : widget(X,Y,W,H)
{
    text = _text;
}

label::~label(void)
{
}

void label::draw(class basecanvas *canvas)
{
    canvas->draw_text(text, X1, Y1, X2-X1, Y2-Y1);
}

void label::handle_event(SDL_Event &event)
{
    /* labels handle no events */
}
