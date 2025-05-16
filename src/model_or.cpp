#include "gridcad.h"
#include "models.h"

model_or::model_or(float _X, float _Y)  : model_logic2(_X, _Y, "assets/orgate.png")
{
}

model_or::~model_or(void)
{
}

void model_or::calculate(int ttl)
{
    struct value result = {};
    
    if (ttl <= 0)
        return;
    
    result.type = VALUE_TYPE_BOOL;
    result.boolval = (ports[0]->value.boolval || ports[1]->value.boolval);
    
    ports[2]->update_value(&result, ttl -1);
    
}