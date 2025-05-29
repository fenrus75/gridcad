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
#include "model_datascope.h"
#include "port.h"

#include <sys/time.h>

model_datascope::model_datascope(float _X, float _Y)  : element(1, 1, "Output")
{
    sizeX = 4;
    sizeY = 3;    
    X = floorf(_X);
    Y = floorf(_Y);
    
    add_port(-1, 1, "Output", PORT_IN);    
}

model_datascope::~model_datascope(void)
{
}

void model_datascope::drawAt(class canvas *canvas, float X, float Y, int type)
{
    if (selected) {
        canvas->draw_image("assets/datascope_selected.png", X, Y, sizeX, sizeY, Alpha(type));
    } else {
        canvas->draw_image("assets/datascope.png", X, Y, sizeX, sizeY, Alpha(type));
    }
    if (ports[0]->value.type == VALUE_TYPE_INT) {
	char buf[128];
	std::string s;
	sprintf(buf, "%li", ports[0]->value.intval);
	s = buf;
	canvas->draw_text(s, X+ 0.6, Y + 0.6, 2.8,1.8);
    }

    for (auto port : ports) {
        port->drawAt(canvas, X,Y, COLOR_WIRE_SOLID);
        port->draw_wires(canvas);
    }
}



void model_datascope::to_json(json &j)
{
     element::to_json(j);
}
void model_datascope::from_json(json &j)
{
     element::from_json(j);
}

void model_datascope::rotate_ports(void)
{
        
    if (ports[0]->X == -1)
      ports[0]->X = sizeX; 
    else 
      ports[0]->X = -1;
    
    ports[0]->route_wires();
    reseat();
}
