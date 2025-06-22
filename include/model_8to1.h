#pragma once

#include "gridcad.h"
#include "port.h"

class model_8to1 : public element
{
public:
    model_8to1(float X, float Y);
    virtual ~model_8to1(void);
    virtual std::string class_id(void) { return "model_8to1:";};
    
    void drawAt(class basecanvas *canvas, float X, float Y, int type) override;
    std::string get_verilog_main(void) override;
    
    void calculate(int ttl) override;
//    void rotate_ports(void) override;

protected:
    struct value value = {};
};

