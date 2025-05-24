#include "gridcad.h"
#include "widget.h"
#include "truthcanvas.h"

button::button(float X, float Y, float W, float H, std::string _icon, int _action, class truthcanvas *_canvas) : widget(X,Y,W,H)
{
    icon = _icon;
    action = _action;
    canvas = _canvas;
}

button::~button(void)
{
}


void button::draw(class basecanvas *canvas) 
{
    canvas->draw_image(icon, X1, Y1, X2-X1, Y2-Y1, 255, true);    
}

/*
#define ACTION_ADD_INPUT 1
#define ACTION_DEL_INPUT 2
#define ACTION_ADD_OUTPUT 3
#define ACTIOM_DEL_OUTPUT 4
*/
void button::select(void)
{
    switch (action) {
    case ACTION_ADD_INPUT:
        canvas->add_input();
        break;
    case ACTION_DEL_INPUT:
        canvas->del_input();
        break;
    case ACTION_ADD_OUTPUT:
        canvas->add_output();
        break;
    case ACTION_DEL_OUTPUT:
        canvas->del_output();
        break;
    }
}
void button::handle_event(SDL_Event &event)
{
}
