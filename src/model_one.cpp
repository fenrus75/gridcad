#include "gridcad.h"
#include "models.h"

model_one::model_one(float _X, float _Y)  : model_zero(_X, _Y)
{
    struct value value;
    value.boolval = true;
    for (auto port : ports) {
        port->update_value(&value);
    }
}

model_one::~model_one(void)
{
}

void model_one::draw(class scene *scene, int type)
{
    scene->drawBox(X,Y,X+1,Y+1, COLOR_ELEMENT_NORMAL);
    scene->drawCircle(X+1.5, Y+0.5, 0.5, COLOR_VALUE_GREEN);
    for (auto wire : wires) {
        wire->draw(scene);
    }
}

