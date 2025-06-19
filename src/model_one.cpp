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
#include "model_one.h"
#include "port.h"

model_one::model_one(float _X, float _Y)  : model_zero(_X, _Y)
{
    struct value value = {};
    
    memset(&value, 0, sizeof(struct value));
    value.boolval = true;
    value.valid = true;
    sizeX = 1;
    sizeY = 1;

    ports.clear();

    add_port(1, 0, "ONE", PORT_OUT, 1);    

    for (auto port : ports) {
        update_value_net(&value, 0, DEFAULT_TTL);
        port->update_value_final(&value, DEFAULT_TTL);
    }
}

model_one::~model_one(void)
{
}

void model_one::drawAt(class canvas *canvas, float X, float Y, int type)
{
    if (selected) {
        canvas->draw_image("assets/one_base.png", X, Y, sizeX, sizeY, Alpha(type));
    } else {	
        canvas->draw_image("assets/one.png", X, Y, sizeX, sizeY, Alpha(type));
    }
    
    for (auto port : ports) {
        port->draw_wires(canvas);
        if (type != DRAW_NORMAL)
            port->drawAt(canvas, X, Y, type);
    }
}

std::string model_one::get_verilog_main(void)
{
    std::string s = "";

    s = "assign "  + ports[0]->get_net_verilog_name() + " = 1'b1;\n";
    
    return s;
}
