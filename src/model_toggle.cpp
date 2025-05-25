#include "gridcad.h"
#include "model_toggle.h"
#include "port.h"

model_toggle::model_toggle(float _X, float _Y)  : element(1, 1, "Input")
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
    if (selected) {
        canvas->draw_image("assets/toggle_base.png", X, Y, sizeX, sizeY, Alpha(type));
    } else if (value.boolval) {
        canvas->draw_image("assets/toggle_on.png", X, Y, sizeX, sizeY, Alpha(type));
    } else {	
        canvas->draw_image("assets/toggle_off.png", X, Y, sizeX, sizeY, Alpha(type));
    }
    
    if (name != "") {
      canvas->draw_text(name, X, Y + sizeY, sizeX, 1);
    }
//    canvas->draw_circle(X+1.5, Y+1.5, 1.5, COLOR_WIRE_SOLID, value_color(&value));
    for (auto port : ports) {
        port->drawAt(canvas, X,Y, COLOR_WIRE_SOLID);
        port->draw_wires(canvas);
    }
}



bool model_toggle::mouse_select(float _X, float _Y)
{
    /* convert _X and _Y to be relative to the center of the model */
    _X = _X - X - sizeX/2.0;
    _Y = _Y - Y - sizeY/2.0;
    
    if ( (_X * _X) + (_Y * _Y) > 1.0)  /* click is outside center button */
         return false;
    value.boolval = !value.boolval;
    value.valid = true;
    printf("TOGGLE to %i\n", value.boolval);
    ports[0]->update_value(&value, DEFAULT_TTL);
    notify(DEFAULT_TTL);
    return true;
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
