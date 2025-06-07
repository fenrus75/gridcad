#pragma once

#include "gridcad.h"



class model_zero : public element
{
public:
    model_zero(float X, float Y);
    virtual ~model_zero(void);
    virtual std::string class_id(void) { return "model_zero:";};
    
    void drawAt(class canvas *canvas, float X, float Y, int type) override;
    void fill_grid(class wiregrid* grid) override;
    bool intersect(float _X, float _Y) override;
    std::string get_verilog_main(void) override;
protected:
};


