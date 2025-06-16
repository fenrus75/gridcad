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
#include "truthcanvas.h"

slider::slider(float X, float Y, float W, float H, std::string _desc, bool *value) : widget(X,Y,W,H)
{
    description = _desc;
    origin = value;
}

slider::~slider(void)
{
}


void slider::draw(class truthcanvas *canvas) 
{
    if (origin && *origin)
        canvas->draw_image("assets/slider_on.png", X1, Y1, 2, 1, 255, true);    
    else
        canvas->draw_image("assets/slider_off.png", X1, Y1, 2, 1, 255, true);
        
    canvas->draw_text_left(description, X1+2.5,Y1,X2-X1-2.5,1);
}

void slider::select(void)
{
    if (!origin)
        return;
    *origin = !*origin;
}
