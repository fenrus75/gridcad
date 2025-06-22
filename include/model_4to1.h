#pragma once

#include "gridcad.h"
#include "port.h"

class model_4to1 : public element
{
public:
    model_4to1(float X, float Y);
    virtual ~model_4to1(void);
    virtual std::string class_id(void) { return "model_4to1:";};
    
    void drawAt(class basecanvas *canvas, float X, float Y, int type) override;
    std::string get_verilog_main(void) override;
    
    void calculate(int ttl) override;

protected:
    struct value value = {};
};

