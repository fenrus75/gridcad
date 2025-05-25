#include "gridcad.h"

#include "widget.h"
#include <sys/time.h>

label::label(float X, float Y, float W, float H, std::string *_text) : widget(X,Y,W,H)
{
    text = _text;
}

label::~label(void)
{
}

void label::draw(class truthcanvas *canvas)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    if (_selected && tv.tv_usec > 500000) 
        canvas->draw_text(*text+"|", X1, Y1, X2-X1, Y2-Y1);
    else
        canvas->draw_text(*text+" ", X1, Y1, X2-X1, Y2-Y1);
}

void label::handle_event(SDL_Event &event)
{
        if (!_selected)
            return;

        /* first handle all generic string based keys */            
        labelevent(event, text);
        switch (event.type) {
	case SDL_KEYDOWN:        
        	switch (event.key.keysym.sym) {
                case SDLK_RETURN:
                    _selected = false;
                    break;
                }
                break;
        }
}
