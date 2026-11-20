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

class model_reg : public element
{
public:
    model_reg(float X, float Y, int width = 8, uint64_t reset_val = 0);
    std::string class_id(void) override { return "model_reg:"; }

    void drawAt(class basecanvas *canvas, float X, float Y, int type) override;
    void calculate(int ttl) override;
    void queued_calculate(int ttl) override;
    void to_json(json &j) override;
    void from_json(json &j) override;
    std::string get_verilog_main(void) override;

    void set_params(int width, uint64_t reset_val);
    int get_bus_width(void) const { return bus_width; }
    uint64_t get_reset_value(void) const { return reset_value; }

private:
    int bus_width = 8;
    uint64_t reset_value = 0;
    struct value stored_value = {};
    struct value previous_clock = {};
    bool prev_rst = false;
};
