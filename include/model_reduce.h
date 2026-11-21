/*
 * gridcad -- a tool to do digital circuit design with a GUI
 *
 * (C) Copyright 2025  -- Arjan van de Ven
 *
 * Licensed under the terms of the
 * GNU General Public LIcense version 3.0
 */

#pragma once

#include "gridcad.h"
#include "element.h"

enum class reduce_op : int {
    OR = 0,
    AND = 1,
    XOR = 2,
    XNOR = 3,
};

class model_reduce : public element
{
public:
    model_reduce(float X, float Y, int width = 8, reduce_op op = reduce_op::OR);
    std::string class_id(void) override { return "model_reduce:"; }

    void drawAt(class basecanvas *canvas, float X, float Y, int type) override;
    void calculate(int ttl) override;
    void to_json(json &j) override;
    void from_json(json &j) override;
    std::string get_verilog_main(void) override;

    void set_bus_width(int width);
    int get_bus_width(void) const { return bus_width; }
    void set_op(reduce_op new_op) { op = new_op; }
    reduce_op get_op(void) const { return op; }

private:
    int bus_width = 8;
    reduce_op op = reduce_op::OR;
};
