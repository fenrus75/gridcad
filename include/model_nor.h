#pragma once

#include "gridcad.h"
#include "model_logic2.h"


class model_nor : public model_logic2
{
public:
    model_nor(float X, float Y);
    virtual ~model_nor(void);
    virtual std::string class_id(void) { return "model_nor:";};    
    void calculate(int ttl) override;
};

