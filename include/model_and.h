#pragma once

#include "gridcad.h"

#include "model_logic2.h"

class model_and : public model_logic2
{
public:
    model_and(float X, float Y);
    virtual ~model_and(void);
    virtual std::string class_id(void) { return "model_and:";};
    
    void calculate(int ttl) override;
};

