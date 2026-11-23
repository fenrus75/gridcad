#pragma once

#include "gridcad.h"
#include "element.h"

#include "model_zero.h"

class model_one : public element
{
public:
    model_one(float X, float Y);
    std::string class_id(void) override { return "model_one:";};

    void drawAt(class basecanvas *canvas, float X, float Y, int type) override;
    std::string get_verilog_main(void) override;
};

