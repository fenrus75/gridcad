/*
 * gridcad -- a tool to do digital circuit design with a GUI
 *
 * (C) Copyright 2025  -- Arjan van de Ven
 *
 * Licensed under the terms of the
 * GNU General Public LIcense version 3.0
 *
 * Authors:
 *    Arjan van de Ven <arjanvandeven@gmail.com>
 */

#include "gridcad.h"

void labelevent(SDL_Event &event, std::string *text)
{
        char c;
        switch (event.type) {
	case SDL_KEYDOWN:
        	switch (event.key.keysym.sym) {
		case SDLK_BACKSPACE:
			if (text->size()>0)
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
                case SDLK_KP_PLUS:             
                    c = '+';
                    *text += c;
                    break;
                case SDLK_MINUS:                
                case SDLK_KP_MINUS:
                    c = '-';
                    if (event.key.keysym.mod & (KMOD_LSHIFT))
                        c = '_';
                    *text += c;
                    break;
                case SDLK_EQUALS:
                    c= '=';
                    if (event.key.keysym.mod & (KMOD_LSHIFT))
                        c = '+';
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
                    break;
                }
                break;
        }
}