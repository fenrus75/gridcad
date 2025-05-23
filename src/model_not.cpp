#include "gridcad.h"
#include "model_not.h"

model_not::model_not(float _X, float _Y)  : element(_X, _Y, "")
{
    ports.resize(0);
    
    sizeX=3;
    sizeY=3;
    
    add_port(-1, 1, "In", PORT_IN);
    add_port(sizeX, 1, "Out", PORT_OUT);
}

model_not::~model_not(void)
{
}

void model_not::drawAt(class canvas *canvas, float X, float Y, int type)
{
    if (!visual) {
        visual = canvas->load_image("assets/inverter.png");    
        visual_selected = canvas->load_image("assets/inverter_selected.png");    
    }
    if (selected)
        canvas->draw_image(visual_selected, X, Y, sizeX, sizeY, Alpha(type));
    else
        canvas->draw_image(visual, X, Y, sizeX, sizeY, Alpha(type));
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
    result.valid = true;
    
    ports[1]->update_value(&result, ttl -1);
}
