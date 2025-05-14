#include "gridcad.h"
#include "models.h"

model_logic2::model_logic2(float _X, float _Y, const char *_filename)  : element(_X, _Y, NULL)
{
    ports.resize(0);
    
    sizeX=5;
    sizeY=3;
    
    filename = strdup(_filename);
    add_port(0, 0, "In1", PORT_IN);
    add_port(0, 2, "In2", PORT_IN);
    add_port(sizeX, 1, "Out", PORT_OUT);
}

model_logic2::~model_logic2(void)
{
}

void model_logic2::drawAt(class canvas *canvas, float X, float Y, int type)
{
    if (!visual) {
        visual = canvas->load_image(filename);    
    }
    canvas->draw_image(visual, X+1, Y, sizeX-1, sizeY, Alpha(type));
    for (auto port: ports) {
        port->drawAt(canvas, X, Y, type);
    }
}


void model_logic2::calculate(void)
{
}
