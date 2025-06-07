#pragma once

#include "gridcad.h"
#include "port.h"

class model_4to8 : public element
{
public:
    model_4to8(float X, float Y);
    virtual ~model_4to8(void);
    virtual std::string class_id(void) { return "model_4to8:";};
    
    void drawAt(class canvas *canvas, float X, float Y, int type) override;
    
    void calculate(int ttl) override;
    std::string get_verilog_main(void) override;

protected:
    struct value value = {};
};

