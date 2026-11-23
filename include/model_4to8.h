#pragma once

#include "gridcad.h"
#include "port.h"

class model_4to8 : public element
{
public:
    model_4to8(float X, float Y);
    std::string class_id(void) override { return "model_4to8:";};
    
    void drawAt(class basecanvas *canvas, float X, float Y, int type) override;
    
    void calculate(int ttl) override;
    std::string get_verilog_main(void) override;

protected:
    struct value value = {};
};

