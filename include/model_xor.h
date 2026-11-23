#pragma once

#include "gridcad.h"
#include "model_logic2.h"

class model_xor : public model_logic2
{
public:
    model_xor(float X, float Y);
    std::string class_id(void) override { return "model_xor:";};    
    void calculate(int ttl) override;
    std::string get_verilog_operand(void) override { return "^"; };
};

