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

/* Parameterized bus splitter: fans an IN_WIDTH bus out to out0..out{N-1}. */
class model_split : public element
{
public:
    model_split(float _X, float _Y, int in_width = 8, int outputs = 8,
                const std::vector<int> &offsets = {},
                const std::vector<int> &out_widths = {});

    std::string class_id(void) override { return "model_split:"; }

    void drawAt(class basecanvas *canvas, float X, float Y, int type) override;
    void calculate(int ttl) override;
    void to_json(json &j) override;
    void from_json(json &j) override;

private:
    int in_width = 8;
    std::vector<int> bit_offsets;
    std::vector<int> output_widths;
};
