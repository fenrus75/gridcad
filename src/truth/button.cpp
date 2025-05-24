#include "gridcad.h"
#include "widget.h"

button::button(float X, float Y, float W, float H, std::string _icon, int _action) : widget(X,Y,W,H)
{
    icon = _icon;
    action = _action;
}

button::~button(void)
{
}


void button::draw(class basecanvas *canvas) 
{
    canvas->draw_image(icon, X1, Y1, X2, Y2, 255, true);    
}

void button::handle_event(SDL_Event &event)
{
}