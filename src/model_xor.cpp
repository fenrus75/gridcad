#include "gridcad.h"
#include "models.h"

model_xor::model_xor(float _X, float _Y)  : model_logic2(_X, _Y, "assets/xorgate.png")
{
    selected_filename = "assets/xorgate_selected.png";
}

model_xor::~model_xor(void)
{
}

void model_xor::calculate(int ttl)
{
    struct value result = {};
    
    if (ttl <= 0)
        return;
    
    result.type = VALUE_TYPE_BOOL;
    result.boolval = (ports[0]->value.boolval ^ ports[1]->value.boolval);
    
    ports[2]->update_value(&result, ttl -1);
    
}