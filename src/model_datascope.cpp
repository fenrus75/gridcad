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


void callback_decimal(class element *element)
{
     class model_datascope *e = (class model_datascope *)element;
     e->set_format(0);
}
void callback_hexadecimal(class element *element)
{
     class model_datascope *e = (class model_datascope *)element;
     e->set_format(1);
}
void callback_binary(class element *element)
{
     class model_datascope *e = (class model_datascope *)element;
     e->set_format(2);
}

model_datascope::model_datascope(float _X, float _Y)  : element(1, 1, "Output")
{
    sizeX = 4;
    sizeY = 3;    
    X = floorf(_X);
    Y = floorf(_Y);
    
    add_port(-1, 1, "Output", PORT_IN);    
    menu->add_item("Decimal", callback_decimal);
    menu->add_item("Hex", callback_hexadecimal);
    menu->add_item("Binary", callback_binary);
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
	if (format == 0)
          sprintf(buf, "%li", ports[0]->value.intval);
	if (format == 1)
          sprintf(buf, "x%lx", ports[0]->value.intval);
	if (format == 2)
          sprintf(buf, "%lb", ports[0]->value.intval);
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
     j["format"] = format;
}
void model_datascope::from_json(json &j)
{
     element::from_json(j);
     format = j.value("format", 0);
}

void model_datascope::rotate_ports(void)
{
        
    if (ports[0]->X == -1) {
      ports[0]->X = 1;
      ports[0]->Y = -1;
    } else if (ports[0]->Y == -1) {
      ports[0]->X = sizeX;
      ports[0]->Y = 1;
    } else if (ports[0]->X == sizeX) {
      ports[0]->X = 1;
      ports[0]->Y = sizeY;
    } else {
      ports[0]->X = -1;
      ports[0]->Y = 1;;
    }
    ports[0]->screenX = X + ports[0]->X;
    ports[0]->screenY = Y + ports[0]->Y;
    
    ports[0]->route_wires();
    reseat();
}
