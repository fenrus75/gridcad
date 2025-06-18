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
#include "model_or.h"

model_or::model_or(float _X, float _Y)  : model_logic2(_X, _Y, "assets/orgate.png")
{
    selected_filename = "assets/orgate_selected.png";
}

model_or::~model_or(void)
{
}

void model_or::calculate(int ttl)
{
    struct value result = {};
    
    if (ttl <= 0)
        return;
    
    result.type = VALUE_TYPE_BOOL;
    result.boolval = (ports[0]->value.boolval || ports[1]->value.boolval);
    result.valid = true;

    update_value_net(&result, 2, ttl - 1);
}