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
        char c;
        if (!_selected)
            return;
        switch (event.type) {
	case SDL_KEYDOWN:
        	switch (event.key.keysym.sym) {
		case SDLK_BACKSPACE:
		    text->pop_back();
                    break;    
                case SDLK_0:
                case SDLK_1:
                case SDLK_2:
                case SDLK_3:
                case SDLK_4:
                case SDLK_5:
                case SDLK_6:
                case SDLK_7:
                case SDLK_8:
                case SDLK_9:                
                    c = '0';
                    c += event.key.keysym.sym - SDLK_0;
                    *text += c;
                    break;
                case SDLK_UNDERSCORE:                
                    c = '_';
                    *text += c;
                    break;
                case SDLK_PLUS:                
                    c = '+';
                    *text += c;
                    break;
                case SDLK_MINUS:                
                    c = '-';
                    *text += c;
                    break;
                case SDLK_a:
                case SDLK_b:
                case SDLK_c:
                case SDLK_d:
                case SDLK_e:
                case SDLK_f:
                case SDLK_g:
                case SDLK_h:
                case SDLK_i:
                case SDLK_j:                
                case SDLK_k:
                case SDLK_l:
                case SDLK_m:
                case SDLK_n:
                case SDLK_o:
                case SDLK_p:
                case SDLK_q:
                case SDLK_r:
                case SDLK_s:
                case SDLK_t:                
                case SDLK_u:
                case SDLK_v:
                case SDLK_w:
                case SDLK_x:
                case SDLK_y:
                case SDLK_z:
                    c= 'a';
                    if (event.key.keysym.mod & (KMOD_LSHIFT))
                        c = 'A';
                    c += event.key.keysym.sym - SDLK_a;
                    *text += c;
                    break;
                case SDLK_RETURN:
                    _selected = false;
                    break;
                }
                break;
        }
}
