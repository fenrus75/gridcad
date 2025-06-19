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
#include "model_8to1.h"
#include "port.h"

#include <sys/time.h>
#include <math.h>
model_8to1::model_8to1(float _X, float _Y)  : element(_X, _Y, "")
{
    ports.resize(0);
    
    sizeX=2;
    sizeY=10;
    
    add_port(sizeX, 1, "Bit0", PORT_OUT);
    add_port(sizeX, 2, "Bit1", PORT_OUT);
    add_port(sizeX, 3, "Bit2", PORT_OUT);
    add_port(sizeX, 4, "Bit3", PORT_OUT);
    add_port(sizeX, 5, "Bit4", PORT_OUT);
    add_port(sizeX, 6, "Bit5", PORT_OUT);
    add_port(sizeX, 7, "Bit6", PORT_OUT);
    add_port(sizeX, 8, "Bit7", PORT_OUT);
    add_port(-1, 5, "0-7", PORT_IN, 8);
}

model_8to1::~model_8to1(void)
{
}

void model_8to1::drawAt(class canvas *canvas, float X, float Y, int type)
{
    int alpha = Alpha(type);

    if (selected) {
        canvas->draw_image_rotated("assets/8to1_selected.png", X, Y, sizeX, sizeY, alpha, angle);
    } else {	
        canvas->draw_image_rotated("assets/8to1.png", X, Y, sizeX , sizeY, alpha, angle);
    }
    hover_ports(canvas);
    for (auto port: ports) {
        port->drawAt(canvas, X, Y, type);
	port->draw_wires(canvas);
    }
}


void model_8to1::calculate(int ttl)
{
    struct value out = {};
    
    if (ttl <= 1) {
        printf("8to1 ttl exceeded\n");
        return;
    }
    out.valid = true;
    out.type = VALUE_TYPE_BOOL;
    for (unsigned int i = 0; i < ports.size(); i++) {
        if (ports[i]->direction == PORT_IN)
            value = ports[i]->value;
    }
    for (unsigned int i = 0; i < ports.size(); i++) {
        
        out.valid = true;
        out.type = VALUE_TYPE_BOOL;
        out.boolval = false;
        if ((value.intval & (1<<i)) && ports[i]->direction == PORT_OUT) {
            out.boolval = true;
	}
        if (ports[i]->direction == PORT_OUT) {
            update_value_net(&out, i, ttl - 1);

        }
    }
    
}


std::string model_8to1::get_verilog_main(void)
{
    std::string s = "";

    for (unsigned int i = 0; i < 8 ; i++) {
        s = s + "assign " + ports[i]->get_net_verilog_name() + " = " + ports[8]->get_net_verilog_name("8'b1") + "["+std::to_string(i)+"];\n";
    }

    
    return s;
}
