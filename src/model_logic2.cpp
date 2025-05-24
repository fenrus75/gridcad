#include "gridcad.h"
#include "model_logic2.h"
#include "port.h"

#include <sys/time.h>
#include <math.h>
model_logic2::model_logic2(float _X, float _Y, std::string _filename)  : element(_X, _Y, "")
{
    ports.resize(0);
    
    sizeX=4;
    sizeY=3;
    
    filename = _filename;
    add_port(-1, 0, "In1", PORT_IN);
    add_port(-1, 2, "In2", PORT_IN);
    add_port(sizeX, 1, "Out", PORT_OUT);
}

model_logic2::~model_logic2(void)
{
}

void model_logic2::drawAt(class canvas *canvas, float X, float Y, int type)
{
    int alpha = Alpha(type);

    if (selected) {
        canvas->draw_image(selected_filename, X, Y, sizeX, sizeY, alpha);
    } else {	
        canvas->draw_image(filename, X, Y, sizeX, sizeY, alpha);
    }
    for (auto port: ports) {
        port->drawAt(canvas, X, Y, type);
    }
}


void model_logic2::calculate(int ttl)
{
}
