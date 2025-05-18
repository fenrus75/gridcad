#include "gridcad.h"
#include "models.h"

#include <sys/time.h>
#include <math.h>
model_logic2::model_logic2(float _X, float _Y, const char *_filename)  : element(_X, _Y, NULL)
{
    ports.resize(0);
    
    sizeX=4;
    sizeY=3;
    
    filename = strdup(_filename);
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
    if (!visual) {
        visual = canvas->load_image(filename);    
        if (selected_filename)
            visual_selected = canvas->load_image(selected_filename);
    }
    if (selected && visual_selected) {
        canvas->draw_image(visual_selected, X, Y, sizeX, sizeY, alpha);
    } else {	
        canvas->draw_image(visual, X, Y, sizeX, sizeY, alpha);
    }
    for (auto port: ports) {
        port->drawAt(canvas, X, Y, type);
    }
}


void model_logic2::calculate(int ttl)
{
}
