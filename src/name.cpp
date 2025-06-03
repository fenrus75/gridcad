#include "gridcad.h"
#include "name.h"

#include <sys/time.h>

name::name(std::string *_val)
{
    value = _val;
    cursorpos = value->size();
}

name::~name(void)
{
}


void name::drawAt(class canvas *canvas, float X, float Y, float W)
{
    if (edit_mode) {
          struct timeval tv;
          gettimeofday(&tv, NULL);
          std::string before, after;
          if (cursorpos < value->size()) {
              before = value->substr(0, cursorpos);
              after = value->substr(cursorpos, std::string::npos);
         } else {
             before = *value;
             after = "";
         };
          if (tv.tv_usec > 500000)
             canvas->draw_text(before + "|" + after, X, Y, W, 1);
          else
             canvas->draw_text(before + " " + after, X, Y, W, 1);
    } else {
      canvas->draw_text(*value + " ", X, Y, W, 1);
    }
}

void name::handle_event(SDL_Event &event)
{
        char c;
        switch (event.type) {
	case SDL_KEYDOWN:
        	switch (event.key.keysym.sym) {
		case SDLK_BACKSPACE:
			if (value->size()>0) {
			    value->pop_back();
                            cursorpos--;
                        }
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
                    insert_char(c);
                    break;
                case SDLK_UNDERSCORE:                
                    c = '_';
                    insert_char(c);
                    break;
                case SDLK_PLUS:   
                case SDLK_KP_PLUS:             
                    c = '+';
                    insert_char(c);
                    break;
                case SDLK_MINUS:                
                case SDLK_KP_MINUS:
                    c = '-';
                    if (event.key.keysym.mod & (KMOD_LSHIFT))
                        c = '_';
                    insert_char(c);
                    break;
                case SDLK_EQUALS:
                    c= '=';
                    if (event.key.keysym.mod & (KMOD_LSHIFT))
                        c = '+';
                    insert_char(c);
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
                    insert_char(c);
                    break;
                case SDLK_RIGHT:
                    cursorpos++;
                    if (cursorpos >= value->size())
                        cursorpos = value->size();
                    break;
                case SDLK_LEFT:
                    if (cursorpos > 0)
                        cursorpos--;
                    break;
                case SDLK_RETURN:
                    break;
                }
                break;
        }
}

void name::insert_char(unsigned char c)
{
    std::string before, after;
    char buffer[2];
    
    printf("Len is %lu cursorpos is %u \n", value->size(), cursorpos);
    
    if (cursorpos >= value->size()) {
        *value += c;
        cursorpos++;
        return;
    }
    before = value->substr(0, cursorpos);
    after = value->substr(cursorpos, std::string::npos);
    buffer[0] = c;
    buffer[1] = 0;
    after = before + buffer + after;
    *value = after;
    cursorpos++;
}
