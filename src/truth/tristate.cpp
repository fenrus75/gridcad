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
#include "widget.h"


tristate::tristate(float X, float Y, float W, float H, char *_tv, int _gridX, int _gridY, bool _is_input, class truthcanvas *_canvas) : widget(X,Y,W,H)
{
	tv = _tv;
	gridX = _gridX;
	gridY = _gridY;
	is_input = _is_input;
	canvas = _canvas;
}

tristate::~tristate(void)
{
}

void tristate::handle_event(SDL_Event &event)
{
	if (!selected())
		return;
	switch (event.type) {
	case SDL_KEYDOWN:
        	switch (event.key.keysym.sym) {
		case SDLK_0:
			canvas->clear_canX();
			if (is_input) {
				if (*tv == 'X')
					canvas->turn_from_X(gridX, gridY);
			} else {
				*tv = '0';
				canvas->values_changed();
			}
			break;	
		case SDLK_1:
			canvas->clear_canX();
			if (is_input) {
				if (*tv == 'X')
					canvas->turn_from_X(gridX, gridY);
			} else {
				*tv = '1';
				canvas->values_changed();
			}
			break;	
		case SDLK_x:
			canvas->clear_canX();
			if (is_input) {
				canvas->turn_to_X(gridX, gridY);
			} else {
				*tv = 'X';
				canvas->values_changed();
			}
			break;	
		}
	break;
	}
}

void tristate::draw(class truthcanvas *canvas)
{

	if (_selected) {
	        canvas->draw_box(X1 + 0.1, Y1 + 0.1, X2 - 0.1, Y2- 0.1, COLOR_ELEMENT_ORIGIN);
	} else {
		if (canvas->is_canX(gridX, gridY))
		        canvas->draw_box(X1 + 0.1, Y1 + 0.1, X2 - 0.1, Y2- 0.1, COLOR_BACKGROUND_YELLOW);
		else
		        canvas->draw_box(X1 + 0.1, Y1 + 0.1, X2 - 0.1, Y2- 0.1, COLOR_BACKGROUND_MAIN);
	}
	        
	if (is_input) {	        
		if (*tv == '0')
			canvas->draw_image("assets/model_truth/tt_in_zero.png", X1,Y1, X2-X1, Y2-Y1, 255, true);
		else if (*tv == '1')
			canvas->draw_image("assets/model_truth/tt_in_one.png", X1,Y1, X2-X1, Y2-Y1, 255, true);
		else
			canvas->draw_image("assets/model_truth/tt_in_dc.png", X1,Y1, X2-X1, Y2-Y1, 255, true);
	} else {
		if (*tv == '0')
			canvas->draw_image("assets/model_truth/tt_out_zero.png", X1,Y1, X2-X1, Y2-Y1, 255, true);
		else if (*tv == '1')
			canvas->draw_image("assets/model_truth/tt_out_one.png", X1,Y1, X2-X1, Y2-Y1, 255, true);
		else
			canvas->draw_image("assets/model_truth/tt_out_dc.png", X1,Y1, X2-X1, Y2-Y1, 255, true);
	}
}