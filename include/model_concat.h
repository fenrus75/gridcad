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

#include <cstdint>
#include <vector>

/* Parameterized bus concatenation: packs in0 (LSB) .. in{N-1} (MSB) into out. */
class model_concat : public element
{
public:
    model_concat(float _X, float _Y, const std::vector<int> &in_widths = {1, 1}, int out_width = 2);

    virtual std::string class_id(void) override { return "model_concat:"; }

    void drawAt(class basecanvas *canvas, float X, float Y, int type) override;
    void calculate(int ttl) override;
    void to_json(json &j) override;
    void from_json(json &j) override;

private:
    std::vector<int> input_widths;
    int out_width = 2;
};
