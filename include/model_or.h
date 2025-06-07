#pragma once

#include "gridcad.h"
#include "model_logic2.h"

class model_or : public model_logic2
{
public:
    model_or(float X, float Y);
    virtual ~model_or(void);
    virtual std::string class_id(void) { return "model_or:";};    
    void calculate(int ttl) override;
    std::string get_verilog_operand(void) override { return "|"; };
};

