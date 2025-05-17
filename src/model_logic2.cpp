#include "gridcad.h"
#include "models.h"

#include <sys/time.h>
#include <math.h>
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
    int alpha = Alpha(type);
    if (!visual) {
        visual = canvas->load_image(filename);    
    }
    if (selected) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        double msec = tv.tv_usec / 1000000.0;
        msec = msec * 2 * 3.1415;
        alpha = 128 + 64 + 63 * sin(msec);  
    }
    canvas->draw_image(visual, X+1, Y, sizeX-1, sizeY, alpha);
    for (auto port: ports) {
        port->drawAt(canvas, X, Y, type);
    }
}


void model_logic2::calculate(int ttl)
{
}
