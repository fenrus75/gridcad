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
#include "model_xor.h"

model_xor::model_xor(float _X, float _Y)  : model_logic2(_X, _Y, "assets/xorgate.png")
{
    selected_filename = "assets/xorgate_selected.png";
}

void model_xor::calculate(int ttl)
{
    struct value result = {};
    
    if (ttl <= 0)
        return;
    
    if (bus_width > 1) {
        result.type = VALUE_TYPE_INT;
        result.intval = (read_port_u64(0) ^ read_port_u64(1)) & bus_mask();
        result.boolval = (result.intval != 0);
    } else {
        result.type = VALUE_TYPE_BOOL;
        result.boolval = (ports[0]->value.boolval ^ ports[1]->value.boolval);
    }
    result.valid = true;
    
    update_value_net(&result, 2, ttl - 1);    
}