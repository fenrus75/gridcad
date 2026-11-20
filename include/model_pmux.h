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

class model_pmux : public element
{
public:
    model_pmux(float X, float Y, int width = 8, int arms = 2);
    std::string class_id(void) override { return "model_pmux:"; }

    void drawAt(class basecanvas *canvas, float X, float Y, int type) override;
    void calculate(int ttl) override;
    void to_json(json &j) override;
    void from_json(json &j) override;
    std::string get_verilog_main(void) override;

    int get_bus_width(void) const { return bus_width; }
    int get_num_arms(void) const { return num_arms; }

private:
    void rebuild_ports(void);

    int bus_width = 8;
    int num_arms = 2;
};
