#include "gridcad.h"
#include "model_1to4.h"
#include "port.h"

#include <sys/time.h>
#include <math.h>
model_1to4::model_1to4(float _X, float _Y)  : element(_X, _Y, "")
{
    ports.resize(0);
    
    sizeX=2;
    sizeY=6;
    
    add_port(sizeX, 1, "Bit0", PORT_OUT);
    add_port(sizeX, 2, "Bit1", PORT_OUT);
    add_port(sizeX, 3, "Bit2", PORT_OUT);
    add_port(sizeX, 4, "Bit3", PORT_OUT);
    add_port(-1, 3, "Bus", PORT_IN, 4);
}

model_1to4::~model_1to4(void)
{
}

void model_1to4::drawAt(class canvas *canvas, float X, float Y, int type)
{
    int alpha = Alpha(type);

    if (selected) {
        canvas->draw_image("assets/1to4_selected.png", X, Y, sizeX, sizeY, alpha);
    } else {	
        canvas->draw_image("assets/1to4.png", X, Y, sizeX , sizeY, alpha);
    }
    for (auto port: ports) {
        port->drawAt(canvas, X, Y, type);
    }
}


void model_1to4::calculate(int ttl)
{
    struct value out;
    out.valid = true;
    out.type = VALUE_TYPE_BOOL;
    for (unsigned int i = 0; i < ports.size(); i++) {
        if (ports[i]->direction == PORT_IN)
            value = ports[i]->value;
    }
    for (unsigned int i = 0; i < ports.size(); i++) {
        
        out.boolval = false;
        if ((value.intval >> i) & 1)
            out.boolval = true;
        if (ports[i]->direction == PORT_OUT)
            ports[i]->update_value(&out, ttl - 1);
    }
    
}

