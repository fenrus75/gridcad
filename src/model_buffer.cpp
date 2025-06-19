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
#include "model_buffer.h"
#include "port.h"

model_buffer::model_buffer(float _X, float _Y)  : element(_X, _Y, "")
{
    ports.resize(0);
    
    sizeX=3;
    sizeY=3;
    
    add_port(-1, 1, "In", PORT_IN, 1);
    add_port( 1, -1, "En", PORT_IN, 1);
    add_port(sizeX, 1, "", PORT_OUT, 1);
}

model_buffer::~model_buffer(void)
{
}

void model_buffer::drawAt(class canvas *canvas, float X, float Y, int type)
{
    if (selected)
        canvas->draw_image_rotated("assets/buffer_selected.png", X, Y, sizeX, sizeY, Alpha(type), angle);
    else {
        if (ports[1]->value.boolval)
            canvas->draw_image_rotated("assets/buffer_on.png", X, Y, sizeX, sizeY, Alpha(type), angle);
        else
            canvas->draw_image_rotated("assets/buffer.png", X, Y, sizeX, sizeY, Alpha(type), angle);
    }
    for (auto port: ports) {
        port->drawAt(canvas, X, Y, type);
    }
}


void model_buffer::calculate(int ttl)
{
    struct value result = {};
    
    if (ttl <= 0)
        return;
    
    result.type = VALUE_TYPE_BOOL;
    result.boolval = ports[0]->value.boolval;
    
    if (ports[1]->value.boolval) {
        ports[2]->direction = PORT_OUT;
        result.valid = true;
        update_value_net(&result, 2, ttl - 1);
    } else {
        ports[2]->direction = PORT_Z;
        result.valid = false;
    }

    ports[2]->update_distances();    
    ports[2]->check_reverse();
}


std::string model_buffer::get_verilog_main(void)
{
    std::string s = "";

    s = "assign "  + ports[2]->get_net_verilog_name() + " = " + ports[1]->get_net_verilog_name() + "?" + ports[0]->get_net_verilog_name() + ": 1'bz;\n";
    
    return s;
}
