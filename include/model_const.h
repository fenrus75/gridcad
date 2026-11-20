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
#include "value.h"

class model_const : public element
{
public:
    model_const(float X, float Y, int width = 8, uint64_t val = 0);
    std::string class_id(void) override { return "model_const:"; }

    void drawAt(class basecanvas *canvas, float X, float Y, int type) override;
    void calculate(int ttl) override;
    void to_json(json &j) override;
    void from_json(json &j) override;
    std::string get_verilog_main(void) override;

    void set_const(int width, uint64_t val);
    int get_bus_width(void) const { return bus_width; }
    uint64_t get_const_value(void) const { return const_value; }

private:
    int bus_width = 8;
    uint64_t const_value = 0;
};
