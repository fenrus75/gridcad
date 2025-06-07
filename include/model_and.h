#pragma once

#include "gridcad.h"

#include "model_logic2.h"

class model_and : public model_logic2
{
public:
    model_and(float X, float Y);
    virtual ~model_and(void);
    std::string class_id(void) override { return "model_and:";};
    std::string get_verilog_operand(void) override { return "&"; };
    
    void calculate(int ttl) override;
};

