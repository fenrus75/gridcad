#include "gridcad.h"
#include "model_8to4.h"
#include "port.h"

#include <sys/time.h>
#include <math.h>
model_8to4::model_8to4(float _X, float _Y)  : element(_X, _Y, "")
{
    ports.resize(0);
    
    sizeX=2;
    sizeY=6;
    
    add_port(sizeX, 1, "0-3", PORT_OUT, 4);
    add_port(sizeX, 4, "4-7", PORT_OUT, 4);
    add_port(-1, 3, "Bus", PORT_IN, 8);
}

model_8to4::~model_8to4(void)
{
}

void model_8to4::drawAt(class canvas *canvas, float X, float Y, int type)
{
    int alpha = Alpha(type);

    if (selected) {
        canvas->draw_image("assets/8to4_selected.png", X, Y, sizeX, sizeY, alpha);
    } else {	
        canvas->draw_image("assets/8to4.png", X, Y, sizeX , sizeY, alpha);
    }
    hover_ports(canvas);
    for (auto port: ports) {
        port->drawAt(canvas, X, Y, type);
	port->draw_wires(canvas);
    }
}


void model_8to4::calculate(int ttl)
{
    struct value out = {};
    
    if (ttl <= 1) {
        printf("8to4 ttl exceeded\n");
        return;
    }
    out.valid = true;
    out.type = VALUE_TYPE_INT;

    value = ports[2]->value;

    out = value;
    out.intval = out.intval >> 4;
    ports[0]->update_value(&out, ttl-1);
    out = value;
    out.intval &= 7;
    ports[1]->update_value(&out, ttl-1);

}

