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
#include "model_8to16.h"
#include "port.h"

#include <sys/time.h>
#include <math.h>
model_8to16::model_8to16(float _X, float _Y)  : element(_X, _Y, "")
{
    ports.resize(0);
    
    sizeX=2;
    sizeY=6;
    
    add_port(-1, 1, "0-7", PORT_IN, 8);
    add_port(-1, 4, "8-15", PORT_IN, 8);
    add_port(sizeX, 3, "0-15", PORT_OUT, 16);
}

model_8to16::~model_8to16(void)
{
}

void model_8to16::drawAt(class canvas *canvas, float X, float Y, int type)
{
    int alpha = Alpha(type);

    if (selected) {
        canvas->draw_image("assets/8to16_selected.png", X, Y, sizeX, sizeY, alpha);
    } else {	
        canvas->draw_image("assets/8to16.png", X, Y, sizeX , sizeY, alpha);
    }
    hover_ports(canvas);
    for (auto port: ports) {
        port->drawAt(canvas, X, Y, type);
	port->draw_wires(canvas);
    }
}


void model_8to16::calculate(int ttl)
{
    struct value out = {};
    
    if (ttl <= 1) {
        printf("8to16 ttl exceeded\n");
        return;
    }
    out.valid = true;
    out.type = VALUE_TYPE_INT;
    
    out.intval = ports[0]->value.intval | (ports[1]->value.intval << 8);

    ports[2]->update_value(&out, ttl-1);

}

std::string model_8to16::get_verilog_main(void)
{
    std::string s = "";
    std::vector<std::string> wiremap;
    std::vector<std::string> wiremap_in1;
    std::vector<std::string> wiremap_in2;
    

    ports[2]->collect_wires(&wiremap);
    ports[0]->collect_wires(&wiremap_in1);
    if (wiremap_in1.size() < 1)
        wiremap_in1.push_back("4'1111");
    ports[1]->collect_wires(&wiremap_in2);
    if (wiremap_in2.size() < 1)
        wiremap_in2.push_back("4'1111");
    
    for (auto wr : wiremap) {
      s = s + "assign " + wr + " = {" + wiremap_in1[0] + ", " + wiremap_in2[0] + "};\n";
    }
    
    return s;
}
