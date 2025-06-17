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
#include "model_logic2.h"
#include "port.h"

#include <sys/time.h>
#include <math.h>
model_logic2::model_logic2(float _X, float _Y, std::string _filename)  : element(_X, _Y, "")
{
    ports.resize(0);
    
    sizeX=4;
    sizeY=3;
    
    filename = _filename;
    add_port(-1, 0, "In1", PORT_IN, 1);
    add_port(-1, 2, "In2", PORT_IN, 1);
    add_port(sizeX, 1, "Out", PORT_OUT, 1);
}

model_logic2::~model_logic2(void)
{
}

void model_logic2::drawAt(class canvas *canvas, float X, float Y, int type)
{
    int alpha = Alpha(type);

    if (selected) {
        canvas->draw_image_rotated(selected_filename, X, Y, sizeX, sizeY, alpha, angle);
    } else {	
        canvas->draw_image_rotated(filename, X, Y, sizeX , sizeY, alpha, angle);
    }
    for (auto port: ports) {
        port->drawAt(canvas, X, Y, type);
    }
}


void model_logic2::calculate(int ttl)
{
}

std::string model_logic2::get_verilog_main(void)
{
    std::string s = "";
    std::vector<std::string> wiremap0, wiremap1, wiremap2;
    
    ports[0]->collect_wires(&wiremap0);
    ports[1]->collect_wires(&wiremap1);
    ports[2]->collect_wires(&wiremap2);
    
    if (wiremap1.size() < 1)
        wiremap1.push_back("1'0");
    if (wiremap0.size() < 1)
        wiremap0.push_back("1'0");
    
    for (auto name : wiremap2) {
        s = "assign "  + name + " = " +wiremap0[0] + " " + get_verilog_operand() + " " + wiremap1[0] + ";\n";
    }
    
    return s;
}