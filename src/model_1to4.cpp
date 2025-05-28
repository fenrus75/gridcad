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
    
    add_port(-1, 1, "Bit0", PORT_IN, 1);
    add_port(-1, 2, "Bit1", PORT_IN, 1);
    add_port(-1, 3, "Bit2", PORT_IN, 1);
    add_port(-1, 4, "Bit3", PORT_IN, 1);
    add_port(sizeX, 3, "Bus", PORT_OUT, 4);
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
    hover_ports(canvas);
    for (auto port: ports) {
        port->drawAt(canvas, X, Y, type);
	port->draw_wires(canvas);
    }
}


void model_1to4::calculate(int ttl)
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

