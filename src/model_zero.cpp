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
    sizeX = 2;
    sizeY = 1;    
    X = floorf(_X);
    Y = floorf(_Y);

    
    memset(&value, 0, sizeof(struct value));
    value.boolval = false;
    value.valid = true;

    ports.clear();

    add_port(1, 0, "ZERO", PORT_OUT, 1);    


    for (auto port : ports) {
        port->update_value(&value, DEFAULT_TTL);
    }

}

model_zero::~model_zero(void)
{
}

void model_zero::drawAt(class canvas *canvas, float X, float Y, int type)
{
    if (selected) {
        canvas->draw_image("assets/zero_base.png", X, Y, sizeX, sizeY, Alpha(type));
    } else {	
        canvas->draw_image("assets/zero.png", X, Y, sizeX, sizeY, Alpha(type));
    }

    for (auto port : ports) {
        port->draw_wires(canvas);
    }
}

void model_zero::fill_grid(class wiregrid *grid)
{
    grid->block_point(X, Y);
    grid->add_soft_cost(X+1, Y - 1, 0.2);
    grid->add_soft_cost(X+1, Y + 1, 0.2);
    grid->add_soft_cost(X, Y - 1, 0.6);
    grid->add_soft_cost(X, Y + 1, 0.6);
    grid->add_soft_cost(X-1, Y - 1, 0.3);
    grid->add_soft_cost(X-1, Y,    0.3);
    grid->add_soft_cost(X-1, Y + 1, 0.3);
}

bool model_zero::intersect(float _X, float _Y)
{
    if (_X >= X  && _Y >= Y  && _X < X + 1 && _Y < Y + sizeY)
        return true;
    return false;
}

std::string model_zero::get_verilog_main(void)
{
    std::string s = "";
    std::vector<std::string> wiremap;
    
    ports[0]->collect_wires(&wiremap);
    
    for (auto name : wiremap) {
        s = "assign "  + name + " = 1'b0;\n";
    }
    
    return s;
}
