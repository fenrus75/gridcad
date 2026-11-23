#pragma once

#include "gridcad.h"


class model_buffer : public element
{
public:
    model_buffer(float X, float Y);
    std::string class_id(void) override { return "model_buffer:";};
    
    void drawAt(class basecanvas *canvas, float X, float Y, int type) override;
    
    void calculate(int ttl) override;
    std::string get_verilog_main(void) override;
protected:
};

