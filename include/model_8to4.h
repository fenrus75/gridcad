#pragma once

#include "gridcad.h"
#include "port.h"

class model_8to4 : public element
{
public:
    model_8to4(float X, float Y);
    virtual ~model_8to4(void);
    virtual std::string class_id(void) { return "model_8to4:";};
    
    void drawAt(class canvas *canvas, float X, float Y, int type) override;
    std::string get_verilog_main(void) override;
    
    void calculate(int ttl) override;
//    void rotate_ports(void) override;

protected:
    struct value value = {};
};

