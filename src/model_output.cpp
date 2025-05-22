#include "gridcad.h"
#include "models.h"

model_output::model_output(float _X, float _Y)  : element(1, 1, "")
{
    sizeX = 3;
    sizeY = 3;    
    X = floorf(_X);
    Y = floorf(_Y);
    
    add_port(-1, 1, "In", PORT_IN);    
}

model_output::~model_output(void)
{
}

void model_output::drawAt(class canvas *canvas, float X, float Y, int type)
{
    if (!visual_on) {
        visual_on = canvas->load_image("assets/output_on.png");    
        visual_off = canvas->load_image("assets/output_off.png");
        visual_selected = canvas->load_image("assets/output_base.png");
    }
    if (selected) {
        canvas->draw_image(visual_selected, X, Y, sizeX, sizeY, Alpha(type));
    } else if (ports[0]->value.boolval) {
        canvas->draw_image(visual_on, X, Y, sizeX, sizeY, Alpha(type));
    } else {	
        canvas->draw_image(visual_off, X, Y, sizeX, sizeY, Alpha(type));
    }
//    canvas->drawCircle(X+1.5, Y+1.5, 1.5, COLOR_WIRE_SOLID, value_color(&value));
    for (auto port : ports) {
        port->drawAt(canvas, X,Y, COLOR_WIRE_SOLID);
        port->draw_wires(canvas);
    }
}



void model_output::to_json(json &j)
{
     element::to_json(j);
}
void model_output::from_json(json &j)
{
     element::from_json(j);
}
