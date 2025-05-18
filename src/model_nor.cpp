#include "gridcad.h"
#include "models.h"

model_nor::model_nor(float _X, float _Y)  : model_logic2(_X, _Y, "assets/norgate.png")
{
    selected_filename = "assets/norgate_selected.png";
}

model_nor::~model_nor(void)
{
}

void model_nor::calculate(int ttl)
{
    struct value result = {};
    
    if (ttl <= 0)
        return;
    
    result.type = VALUE_TYPE_BOOL;
    result.boolval = !(ports[0]->value.boolval || ports[1]->value.boolval);
    
    ports[2]->update_value(&result, ttl -1);
    
}