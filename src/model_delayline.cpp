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
#include "model_delayline.h"
#include "port.h"

model_delayline::model_delayline(float _X, float _Y)  : element(_X, _Y, "")
{
    ports.resize(0);
    
    sizeX=2;
    sizeY=1;
    
    add_port(-1, 0, "In", PORT_IN);
    add_port(sizeX, 0, "Out", PORT_OUT);
}

model_delayline::~model_delayline(void)
{
}

void model_delayline::drawAt(class canvas *canvas, float X, float Y, int type)
{
    if (selected)
        canvas->draw_image_rotated("assets/delayline_selected.png", X, Y, sizeX, sizeY, Alpha(type), angle);
    else
        canvas->draw_image_rotated("assets/delayline.png", X, Y, sizeX, sizeY, Alpha(type), angle);
    for (auto port: ports) {
        port->drawAt(canvas, X, Y, type);
    }
}


void model_delayline::calculate(int ttl)
{
    if (ttl <= 0)
        return;

    value = ports[0]->value;
    printf("calculate %s from %i \n", name.c_str(), value.boolval);
    queue_calculate(this);
    
    if (ttl > DEFAULT_TTL - 10)
        ports[1]->update_value(&value, ttl - 1);
}

void model_delayline::queued_calculate(int ttl)
{
    printf("delated calculate %s from %i \n", name.c_str(), value.boolval);
    ports[1]->update_value(&value, ttl - 1);
}


void model_delayline::rotate_ports(void)
{
    for (auto port : ports) {
        float x,y,_x,_y;
        x = port->X - (sizeX-1)/2.0;
        y = port->Y - (sizeY-1)/2.0;
        
        _x = y;
        _y = -x;

	float tmp = sizeY;
	sizeY = sizeX;
	sizeX = tmp;

        port->X = _x + (sizeX-1)/2.0 ;
        port->Y = _y + (sizeY-1)/2.0;
        port->screenX = X + port->X;
        port->screenY = Y + port->Y;

	tmp = sizeY;
	sizeY = sizeX;
	sizeX = tmp;
        
        port->route_wires();
    }
	float tmp = sizeY;
	sizeY = sizeX;
	sizeX = tmp;
    reseat();
    angle -= 90;
    if (angle >= 360)
	angle -= 360;
    if (angle < 0)
	angle += 360;
}
