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

void model_not::drawAt(class canvas *canvas, float X, float Y, int type)
{
    if (!visual) {
        visual = canvas->load_image("assets/inverter.png");    
    }
    canvas->draw_image(visual, X+1, Y, sizeX-1, sizeY, Alpha(type));
    for (auto port: ports) {
        port->drawAt(canvas, X, Y, type);
    }
}


void model_not::calculate(int ttl)
{
    struct value result = {};
    
    if (ttl <= 0)
        return;
    
    result.type = VALUE_TYPE_BOOL;
    result.boolval = !(ports[0]->value.boolval);
    
    ports[1]->update_value(&result, ttl -1);
}
