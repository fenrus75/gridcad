#pragma once

#include "gridcad.h"
#include "port.h"

class model_16to8 : public element
{
public:
    model_16to8(float X, float Y);
    virtual ~model_16to8(void);
    virtual std::string class_id(void) { return "model_16to8:";};
    
    void drawAt(class basecanvas *canvas, float X, float Y, int type) override;
    
    void calculate(int ttl) override;
    std::string get_verilog_main(void) override;
//    void rotate_ports(void) override;

protected:
    struct value value = {};
};

