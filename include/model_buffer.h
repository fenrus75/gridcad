#pragma once

#include "gridcad.h"


class model_buffer : public element
{
public:
    model_buffer(float X, float Y);
    virtual ~model_buffer(void);
    virtual std::string class_id(void) { return "model_buffer:";};
    
    void drawAt(class canvas *canvas, float X, float Y, int type) override;
    
    void calculate(int ttl) override;
    std::string get_verilog_main(void) override;
protected:
};

