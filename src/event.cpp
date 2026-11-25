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
	case SDL_EVENT_KEY_DOWN:
        	switch (event.key.key) {
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
                    c += event.key.key - SDLK_0;
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
                    if (event.key.mod & (SDL_KMOD_LSHIFT))
                        c = '_';
                    *text += c;
                    break;
                case SDLK_EQUALS:
                    c= '=';
                    if (event.key.mod & (SDL_KMOD_LSHIFT))
                        c = '+';
                    *text += c;
                    break;                
                case SDLK_A:
                case SDLK_B:
                case SDLK_C:
                case SDLK_D:
                case SDLK_E:
                case SDLK_F:
                case SDLK_G:
                case SDLK_H:
                case SDLK_I:
                case SDLK_J:                
                case SDLK_K:
                case SDLK_L:
                case SDLK_M:
                case SDLK_N:
                case SDLK_O:
                case SDLK_P:
                case SDLK_Q:
                case SDLK_R:
                case SDLK_S:
                case SDLK_T:                
                case SDLK_U:
                case SDLK_V:
                case SDLK_W:
                case SDLK_X:
                case SDLK_Y:
                case SDLK_Z:
                    c= 'a';
                    if (event.key.mod & (SDL_KMOD_LSHIFT))
                        c = 'A';
                    c += event.key.key - SDLK_A;
                    *text += c;
                    break;
                case SDLK_RETURN:
                    break;
                }
                break;
        }
}