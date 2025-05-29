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
#include "model_not.h"
#include "port.h"

model_not::model_not(float _X, float _Y)  : element(_X, _Y, "")
{
    ports.resize(0);
    
    sizeX=3;
    sizeY=3;
    
    add_port(-1, 1, "In", PORT_IN, 1);
    add_port(sizeX, 1, "Out", PORT_OUT, 1);
}

model_not::~model_not(void)
{
}

void model_not::drawAt(class canvas *canvas, float X, float Y, int type)
{
    if (selected)
        canvas->draw_image_rotated("assets/inverter_selected.png", X, Y, sizeX, sizeY, Alpha(type), angle);
    else
        canvas->draw_image_rotated("assets/inverter.png", X, Y, sizeX, sizeY, Alpha(type), angle);
    for (auto port: ports) {
        port->drawAt(canvas, X, Y, type);
    }
}


void model_not::calculate(int ttl)
{
    struct value result = {};
    
    if (ttl <= 0)
        return;
    
    result.type = VALUE_TYPE_BOOL;
    result.boolval = !(ports[0]->value.boolval);
    result.valid = true;
    
    ports[1]->update_value(&result, ttl -1);
}


void model_not::rotate_ports(void)
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
