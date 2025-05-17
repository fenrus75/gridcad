#include "gridcad.h"
#include "models.h"

model_toggle::model_toggle(float _X, float _Y)  : element(1, 1, NULL)
{
    sizeX = 3;
    sizeY = 3;    
    X = floorf(_X);
    Y = floorf(_Y);
    
    add_port(sizeX, 1, "Toggle", PORT_OUT);    
}

model_toggle::~model_toggle(void)
{
}

void model_toggle::drawAt(class canvas *canvas, float X, float Y, int type)
{
//    element::drawAt(canvas, X, Y, type);
    canvas->drawCircle(X+1.5, Y+1.5, 1.5, COLOR_WIRE_SOLID, value_color(&value));
    for (auto port : ports) {
        port->drawAt(canvas, X,Y, COLOR_WIRE_SOLID);
        port->draw_wires(canvas);
    }
}



void model_toggle::mouse_select(void)
{
    value.boolval = !value.boolval;
    printf("TOGGLE to %i\n", value.boolval);
    ports[0]->update_value(&value, DEFAULT_TTL);
    notify(DEFAULT_TTL);
}

void model_toggle::to_json(json &j)
{
     element::to_json(j);
     j["value"] = value;   
}
void model_toggle::from_json(json &j)
{
     element::from_json(j);
     value = j["value"];
}
