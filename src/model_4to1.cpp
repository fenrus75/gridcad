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
#include "model_4to1.h"
#include "port.h"

#include <sys/time.h>
#include <math.h>
model_4to1::model_4to1(float _X, float _Y)  : element(_X, _Y, "")
{
    ports.resize(0);
    
    sizeX=2;
    sizeY=6;
    
    add_port(sizeX, 1, "Bit0", PORT_OUT);
    add_port(sizeX, 2, "Bit1", PORT_OUT);
    add_port(sizeX, 3, "Bit2", PORT_OUT);
    add_port(sizeX, 4, "Bit3", PORT_OUT);
    add_port(-1, 3, "0-3", PORT_IN, 4);
}

model_4to1::~model_4to1(void)
{
}

void model_4to1::drawAt(class canvas *canvas, float X, float Y, int type)
{
    int alpha = Alpha(type);

    if (selected) {
        canvas->draw_image_rotated("assets/4to1_selected.png", X, Y, sizeX, sizeY, alpha, angle);
    } else {	
        canvas->draw_image_rotated("assets/4to1.png", X, Y, sizeX , sizeY, alpha, angle);
    }
    hover_ports(canvas);
    for (auto port: ports) {
        port->drawAt(canvas, X, Y, type);
	port->draw_wires(canvas);
    }
}


void model_4to1::calculate(int ttl)
{
    struct value out = {};
    
    if (ttl <= 1) {
        printf("4to1 ttl exceeded\n");
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


std::string model_4to1::get_verilog_main(void)
{
    std::string s = "";
    std::vector<std::string> wiremap;
    std::vector<std::string> wiremap_in;
    

    ports[4]->collect_wires(&wiremap_in);
    if (wiremap_in.size() < 1)
        wiremap_in.push_back("1'b0");

    for (unsigned int i = 0; i < 4; i++) {
        ports[i]->collect_wires(&wiremap);
    
        for (auto wr : wiremap) {
          s = s + "assign " + wr + " = " + wiremap_in[0] + "["+std::to_string(i)+"];\n";
        }
        wiremap.clear();
    }

    
    return s;
}

