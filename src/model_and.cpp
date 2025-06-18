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
#include "model_and.h"

model_and::model_and(float _X, float _Y)  : model_logic2(_X, _Y, "assets/andgate.png")
{
    selected_filename = "assets/andgate_selected.png";
}

model_and::~model_and(void)
{
}

void model_and::calculate(int ttl)
{
    struct value result = {};
    
    if (ttl <= 0)
        return;
    
    result.type = VALUE_TYPE_BOOL;
    result.boolval = (ports[0]->value.boolval && ports[1]->value.boolval);
    result.valid = true;

    update_value_net(&result, 2, ttl - 1);    
}