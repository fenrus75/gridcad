#include "gridcad.h"
#include "models.h"

model_not::model_not(float _X, float _Y)  : element(_X, _Y, NULL)
{
    ports.resize(0);
    
    sizeX=4;
    sizeY=3;
    
    add_port(0, 1, "In", PORT_IN);
    add_port(sizeX, 1, "Out", PORT_OUT);
}

model_not::~model_not(void)
{
}

void model_not::drawAt(class scene *scene, float X, float Y, int type)
{
    if (!visual) {
        visual = scene->load_image("assets/inverter.png");    
    }
    scene->draw_image(visual, X+1, Y, sizeX-1, sizeY, Alpha(type));
    for (auto port: ports) {
        port->drawAt(scene, X, Y, type);
    }
}


void model_not::calculate(void)
{
    struct value result = {};
    
    result.type = VALUE_TYPE_BOOL;
    result.boolval = !(ports[0]->value.boolval);
    
    ports[1]->update_value(&result);
}
