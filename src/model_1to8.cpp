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
#include "model_1to8.h"
#include "port.h"

#include <sys/time.h>
#include <math.h>
model_1to8::model_1to8(float _X, float _Y)  : element(_X, _Y, "1to8")
{
    ports.resize(0);
    
    sizeX=2;
    sizeY=10;
    
    add_port(-1, 1, "Bit0", PORT_IN, 1);
    add_port(-1, 2, "Bit1", PORT_IN, 1);
    add_port(-1, 3, "Bit2", PORT_IN, 1);
    add_port(-1, 4, "Bit3", PORT_IN, 1);
    add_port(-1, 5, "Bit4", PORT_IN, 1);
    add_port(-1, 6, "Bit5", PORT_IN, 1);
    add_port(-1, 7, "Bit6", PORT_IN, 1);
    add_port(-1, 8, "Bit7", PORT_IN, 1);
    add_port(sizeX, 5, "0-7", PORT_OUT, 8);
}

model_1to8::~model_1to8(void)
{
}

void model_1to8::drawAt(class canvas *canvas, float X, float Y, int type)
{
    int alpha = Alpha(type);

    if (selected) {
        canvas->draw_image("assets/1to8_selected.png", X, Y, sizeX, sizeY, alpha);
    } else {	
        canvas->draw_image("assets/1to8.png", X, Y, sizeX , sizeY, alpha);
    }
    hover_ports(canvas);
    for (auto port: ports) {
        port->drawAt(canvas, X, Y, type);
	port->draw_wires(canvas);
    }
}


void model_1to8::calculate(int ttl)
{
    value.valid = true;
    value.type = VALUE_TYPE_INT;
    value.intval = 0;
    for (unsigned int i = 0; i < ports.size(); i++) {
        if (ports[i]->value.boolval && ports[i]->direction == PORT_IN)
            value.intval |= (1 << i);
    }
    for (unsigned int i = 0; i < ports.size(); i++) {
        if (ports[i]->direction == PORT_OUT)
            ports[i]->update_value(&value, ttl - 1);
    }
    
}

std::string model_1to8::get_verilog_main(void)
{
    std::string s = "";
    std::vector<std::string> wiremap;
    std::vector<std::string> wiremap_in1;
    std::vector<std::string> wiremap_in2;
    std::vector<std::string> wiremap_in3;
    std::vector<std::string> wiremap_in4;
    std::vector<std::string> wiremap_in5;
    std::vector<std::string> wiremap_in6;
    std::vector<std::string> wiremap_in7;
    std::vector<std::string> wiremap_in8;
    

    ports[8]->collect_wires(&wiremap);
    ports[0]->collect_wires(&wiremap_in1);
    if (wiremap_in1.size() < 1)
        wiremap_in1.push_back("1'b0");
    ports[1]->collect_wires(&wiremap_in2);
    if (wiremap_in2.size() < 1)
        wiremap_in2.push_back("1'b0");
    ports[2]->collect_wires(&wiremap_in3);
    if (wiremap_in3.size() < 1)
        wiremap_in3.push_back("1'b0");
    ports[3]->collect_wires(&wiremap_in4);
    if (wiremap_in4.size() < 1)
        wiremap_in4.push_back("1'b0");
    ports[4]->collect_wires(&wiremap_in5);
    if (wiremap_in5.size() < 1)
        wiremap_in5.push_back("1'b0");
    ports[5]->collect_wires(&wiremap_in6);
    if (wiremap_in6.size() < 1)
        wiremap_in6.push_back("1'b0");
    ports[6]->collect_wires(&wiremap_in7);
    if (wiremap_in7.size() < 1)
        wiremap_in7.push_back("1'b0");
    ports[7]->collect_wires(&wiremap_in8);
    if (wiremap_in8.size() < 1)
        wiremap_in8.push_back("1'b0");
    
    for (auto wr : wiremap) {
      s = s + "assign " + wr + " = {" + wiremap_in1[0] + ", " + wiremap_in2[0]+ ", " + wiremap_in3[0]+ ", " + wiremap_in4[0] + ", " + wiremap_in5[0] + ", " + wiremap_in6[0]+ ", " + wiremap_in7[0]+ ", " + wiremap_in8[0]+ "};\n";
    }
    
    return s;
}
