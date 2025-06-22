#pragma once

#include "gridcad.h"
#include "element.h"

#include "model_zero.h"

class model_one : public element
{
public:
    model_one(float X, float Y);
    virtual ~model_one(void);
    virtual std::string class_id(void) { return "model_one:";};

    void drawAt(class basecanvas *canvas, float X, float Y, int type) override;
    std::string get_verilog_main(void) override;
};

