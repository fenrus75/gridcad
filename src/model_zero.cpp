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
#include "model_zero.h"
#include "port.h"

model_zero::model_zero(float _X, float _Y)  : element(1, 1, "")
{
    struct value value = {};
    sizeX = 1;
    sizeY = 1;    
    X = floorf(_X);
    Y = floorf(_Y);

    
    memset(&value, 0, sizeof(struct value));
    value.boolval = false;
    value.valid = true;

    add_port(1, 0, "ZERO", PORT_OUT, 0);    


    for (auto port : ports) {
        port->update_value_final(&value, DEFAULT_TTL);
    }

}
model_zero::~model_zero(void)
{
}

void model_zero::drawAt(class canvas *canvas, float X, float Y, int type)
{
    if (selected) {
        canvas->draw_image_rotated("assets/zero_base.png", X, Y, sizeX, sizeY, Alpha(type), angle);
    } else {	
        canvas->draw_image_rotated("assets/zero.png", X, Y, sizeX, sizeY, Alpha(type), angle);
    }

    for (auto port : ports) {
        port->draw_wires(canvas);
        if (type != DRAW_NORMAL)
            port->drawAt(canvas, X, Y, type);
    }
}

std::string model_zero::get_verilog_main(void)
{
    std::string s = "";
    int w = ports[0]->get_net_width();
    if (w == 0) 
        w = 1;

    s = "assign "  + ports[0]->get_net_verilog_name() + " = " + std::to_string(w) +"'b0;\n";
    
    return s;
}
