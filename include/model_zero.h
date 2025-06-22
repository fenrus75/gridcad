#pragma once

#include "gridcad.h"
#include "element.h"


class model_zero : public element
{
public:
    model_zero(float X, float Y);
    virtual ~model_zero(void);
    virtual std::string class_id(void) { return "model_zero:";};
    
    void drawAt(class basecanvas *canvas, float X, float Y, int type) override;
    std::string get_verilog_main(void) override;
protected:
};


