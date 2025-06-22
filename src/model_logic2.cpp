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

void model_logic2::drawAt(class basecanvas *canvas, float X, float Y, int type)
{
    int alpha = Alpha(type);

    if (selected) {
        canvas->draw_image_rotated(selected_filename, X, Y, sizeX, sizeY, alpha, angle);
    } else {	
        canvas->draw_image_rotated(filename, X, Y, sizeX , sizeY, alpha, angle);
    }
    for (auto port: ports) {
        port->draw_wires(canvas);
        port->drawAt(canvas, X, Y, type);
    }
}


void model_logic2::calculate(int ttl)
{
}

std::string model_logic2::get_verilog_main(void)
{
    std::string s = "";
    /* no output connected */
    if (!ports[2]->has_net())
        return "";  
    
    s = "assign "  + ports[2]->get_net_verilog_name() + " = " + ports[0]->get_net_verilog_name() + " " + get_verilog_operand() + " " + ports[1]->get_net_verilog_name() + ";\n";
    
    return s;
}