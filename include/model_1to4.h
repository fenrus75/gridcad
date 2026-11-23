#pragma once

#include "gridcad.h"
#include "port.h"

class model_1to4 : public element
{
public:
    model_1to4(float X, float Y);
    std::string class_id(void) override { return "model_1to4:";};
    
    void drawAt(class basecanvas *canvas, float X, float Y, int type) override;
    
    void calculate(int ttl) override;
    std::string get_verilog_main(void) override;


protected:
    struct value value = {};
};

