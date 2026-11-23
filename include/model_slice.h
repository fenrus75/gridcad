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

/* Parameterized bit-slice extractor: out = in[offset + out_width - 1 : offset]. */
class model_slice : public element
{
public:
    model_slice(float _X, float _Y, int in_width = 8, int out_width = 1, int offset = 0);

    std::string class_id(void) override { return "model_slice:"; }

    void drawAt(class basecanvas *canvas, float X, float Y, int type) override;
    void calculate(int ttl) override;
    void to_json(json &j) override;
    void from_json(json &j) override;

private:
    int in_width = 8;
    int out_width = 1;
    int offset = 0;
};
