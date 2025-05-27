#include "gridcad.h"
#include "model_4to1.h"
#include "port.h"

#include <sys/time.h>
#include <math.h>
model_4to1::model_4to1(float _X, float _Y)  : element(_X, _Y, "")
{
    ports.resize(0);
    
    sizeX=2;
    sizeY=6;
    
    add_port(-1, 1, "Bit0", PORT_IN);
    add_port(-1, 2, "Bit1", PORT_IN);
    add_port(-1, 3, "Bit2", PORT_IN);
    add_port(-1, 4, "Bit3", PORT_IN);
    add_port(sizeX, 3, "Bus", PORT_OUT, 4);
}

model_4to1::~model_4to1(void)
{
}

void model_4to1::drawAt(class canvas *canvas, float X, float Y, int type)
{
    int alpha = Alpha(type);

    if (selected) {
        canvas->draw_image("assets/4to1_selected.png", X, Y, sizeX, sizeY, alpha);
    } else {	
        canvas->draw_image("assets/4to1.png", X, Y, sizeX , sizeY, alpha);
    }
    for (auto port: ports) {
        port->drawAt(canvas, X, Y, type);
    }
}


void model_4to1::calculate(int ttl)
{
    value.valid = true;
    value.type = VALUE_TYPE_INT;
    value.intval = 0;
    for (unsigned int i = 0; i < ports.size(); i++) {
        if (ports[i]->value.boolval && ports[i]->direction == PORT_IN)
            value.intval |= (1 << i);
    }
    for (unsigned int i = 0; i < ports.size(); i++) {
        if (ports[i]->direction == PORT_OUT)
            ports[i]->update_value(&value, ttl - 1);
    }
    
}

