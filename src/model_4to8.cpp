#include "gridcad.h"
#include "model_4to8.h"
#include "port.h"

#include <sys/time.h>
#include <math.h>
model_4to8::model_4to8(float _X, float _Y)  : element(_X, _Y, "")
{
    ports.resize(0);
    
    sizeX=2;
    sizeY=6;
    
    add_port(-1, 1, "0-3", PORT_IN, 4);
    add_port(-1, 4, "4-7", PORT_IN, 4);
    add_port(sizeX, 3, "0-7", PORT_OUT, 8);
}

model_4to8::~model_4to8(void)
{
}

void model_4to8::drawAt(class canvas *canvas, float X, float Y, int type)
{
    int alpha = Alpha(type);

    if (selected) {
        canvas->draw_image("assets/4to8_selected.png", X, Y, sizeX, sizeY, alpha);
    } else {	
        canvas->draw_image("assets/4to8.png", X, Y, sizeX , sizeY, alpha);
    }
    hover_ports(canvas);
    for (auto port: ports) {
        port->drawAt(canvas, X, Y, type);
	port->draw_wires(canvas);
    }
}


void model_4to8::calculate(int ttl)
{
    struct value out = {};
    
    if (ttl <= 1) {
        printf("4to8 ttl exceeded\n");
        return;
    }
    out.valid = true;
    out.type = VALUE_TYPE_INT;
    
    out.intval = ports[0]->value.intval | (ports[1]->value.intval << 4);

    ports[2]->update_value(&out, ttl-1);

}

