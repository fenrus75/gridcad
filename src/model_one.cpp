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

model_one::model_one(float _X, float _Y)  : element(_X, _Y, "")
{
    struct value value = {};
    sizeX = 1;
    sizeY = 1;    
    X = floorf(_X);
    Y = floorf(_Y);

    
    value = {};
    value.boolval = true;
    value.intval = ~0;
    value.valid = true;

    add_port(1, 0, "ONE", PORT_OUT, 0);    


    for (auto &port : ports) {
        port->update_value_final(&value, DEFAULT_TTL);
    }
}

void model_one::drawAt(class basecanvas *canvas, float X, float Y, int type)
{
    if (selected) {
        canvas->draw_image("assets/one_base.png", X, Y, sizeX, sizeY, Alpha(type));
    } else {	
        canvas->draw_image("assets/one.png", X, Y, sizeX, sizeY, Alpha(type));
    }
    
    for (auto &port : ports) {
        port->draw_wires(canvas);
        if (type != DRAW_NORMAL)
            port->drawAt(canvas, X, Y, type);
    }
}

std::string model_one::get_verilog_main(void)
{
    const unsigned int w = ports[0]->get_net_width();
    const std::string ones(w, '1');
    return std::format("assign {} = {}'b{};\n",
                       ports[0]->get_net_verilog_name(), w, ones);
}
