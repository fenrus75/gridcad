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
#include "model_8to4.h"
#include "port.h"

#include <sys/time.h>
#include <math.h>
model_8to4::model_8to4(float _X, float _Y)  : element(_X, _Y, "")
{
    ports.resize(0);
    
    sizeX=2;
    sizeY=6;
    
    add_port(sizeX, 1, "0-3", PORT_OUT, 4);
    add_port(sizeX, 4, "4-7", PORT_OUT, 4);
    add_port(-1, 3, "0-7", PORT_IN, 8);
}

model_8to4::~model_8to4(void)
{
}

void model_8to4::drawAt(class canvas *canvas, float X, float Y, int type)
{
    int alpha = Alpha(type);

    if (selected) {
        canvas->draw_image_rotated("assets/8to4_selected.png", X, Y, sizeX, sizeY, alpha, angle);
    } else {	
        canvas->draw_image_rotated("assets/8to4.png", X, Y, sizeX , sizeY, alpha, angle);
    }
    hover_ports(canvas);
    for (auto port: ports) {
        port->drawAt(canvas, X, Y, type);
	port->draw_wires(canvas);
    }
}


void model_8to4::calculate(int ttl)
{
    struct value out = {};
    
    int val;
    
    if (ttl <= 1) {
        printf("8to4 ttl exceeded\n");
        return;
    }
    out.valid = true;
    out.type = VALUE_TYPE_INT;
    
    val = ports[2]->value.intval;
    out.intval = val >> 4;
    update_value_net(&out, 1, ttl - 1);

    out.intval = val & 15;
    update_value_net(&out, 0, ttl - 1);


}

std::string model_8to4::get_verilog_main(void)
{
    std::string s = "";


    if (ports[0]->has_net())
        s = s + "assign " + ports[0]->get_net_verilog_name() + " = " + ports[2]->get_net_verilog_name() + "[" + std::to_string(ports[0]->get_width()-1) + ":0];\n";
    if (ports[1]->has_net())
        s = s + "assign " + ports[1]->get_net_verilog_name() + " = " + ports[2]->get_net_verilog_name() + "[" + std::to_string(ports[2]->get_width()-1) + ":" + std::to_string(ports[1]->get_width())+"];\n";
    return s;
}
