#pragma once

#include "gridcad.h"
#include "model_logic2.h"

class model_nand : public model_logic2
{
public:
    model_nand(float X, float Y);
    std::string class_id(void) override { return "model_nand:";};
    std::string get_verilog_operand(void) override { return "~&"; };    
    void calculate(int ttl) override;
};

