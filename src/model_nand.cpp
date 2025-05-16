#include "gridcad.h"
#include "models.h"

model_nand::model_nand(float _X, float _Y)  : model_logic2(_X, _Y, "assets/nandgate.png")
{
}

model_nand::~model_nand(void)
{
}

void model_nand::calculate(int ttl)
{
    struct value result = {};
    if (ttl <= 0)
        return;
    
    result.type = VALUE_TYPE_BOOL;
    result.boolval = !(ports[0]->value.boolval && ports[1]->value.boolval);
    
    ports[2]->update_value(&result, ttl -1);
    
}