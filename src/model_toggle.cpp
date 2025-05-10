#include "gridcad.h"
#include "models.h"

model_toggle::model_toggle(float _X, float _Y)  : element(1, 1, NULL)
{
    sizeX = 3;
    sizeY = 3;    
    X = floorf(_X);
    Y = floorf(_Y);
    
    add_port(3, 1, "Toggle", PORT_OUT);    
}

model_toggle::~model_toggle(void)
{
}

void model_toggle::drawAt(class scene *scene, float X, float Y, int type)
{
//    element::drawAt(scene, X, Y, type);
    scene->drawCircle(X+2, Y+1, 1.5, COLOR_WIRE_SOLID, value_color(&value));
    for (auto port : ports) {
        port->drawAt(scene, X,Y, COLOR_WIRE_SOLID);
        port->draw_wires(scene);
    }
}



void model_toggle::mouse_select(void)
{
    value.boolval = !value.boolval;
    printf("TOGGLE to %i\n", value.boolval);
    ports[0]->update_value(&value);
    notify();
}