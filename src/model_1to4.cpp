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
#include "model_1to4.h"
#include "port.h"

#include <sys/time.h>
#include <math.h>
model_1to4::model_1to4(float _X, float _Y)  : element(_X, _Y, "")
{
    ports.resize(0);
    
    sizeX=2;
    sizeY=6;
    
    add_port(-1, 1, "Bit0", PORT_IN, 1);
    add_port(-1, 2, "Bit1", PORT_IN, 1);
    add_port(-1, 3, "Bit2", PORT_IN, 1);
    add_port(-1, 4, "Bit3", PORT_IN, 1);
    add_port(sizeX, 3, "0-3", PORT_OUT, 4);
}

model_1to4::~model_1to4(void)
{
}

void model_1to4::drawAt(class canvas *canvas, float X, float Y, int type)
{
    int alpha = Alpha(type);

    if (selected) {
        canvas->draw_image_rotated("assets/1to4_selected.png", X, Y, sizeX, sizeY, alpha, angle);
    } else {	
        canvas->draw_image_rotated("assets/1to4.png", X, Y, sizeX , sizeY, alpha, angle);
    }
    hover_ports(canvas);
    for (auto port: ports) {
        port->drawAt(canvas, X, Y, type);
	port->draw_wires(canvas);
    }
}


void model_1to4::calculate(int ttl)
{
    value.valid = true;
    value.type = VALUE_TYPE_INT;
    value.intval = 0;
    for (unsigned int i = 0; i < ports.size(); i++) {
        if (ports[i]->value.boolval && ports[i]->direction == PORT_IN)
            value.intval |= (1 << i);
    }
    for (unsigned int i = 0; i < ports.size(); i++) {
        if (ports[i]->direction == PORT_OUT) {
	    update_value_net(&value, i, ttl - 1);
	}
    }
    
}

std::string model_1to4::get_verilog_main(void)
{
    std::string s = "";

    s = s + "assign " + ports[4]->get_net_verilog_name() + " = {" + ports[0]->get_net_verilog_name("1'b0") + ", " + ports[1]->get_net_verilog_name("1'b0") + ", " + ports[2]->get_net_verilog_name("1'b0") + ", " + ports[3]->get_net_verilog_name("1'b0") + "};\n";

    return s;
}
