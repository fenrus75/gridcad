#include "gridcad.h"
#include "model_toggle.h"
#include "port.h"

#include <sys/time.h>

model_toggle::model_toggle(float _X, float _Y)  : element(1, 1, "Input")
{
    sizeX = 3;
    sizeY = 3;    
    X = floorf(_X);
    Y = floorf(_Y);
    
    add_port(sizeX, 1, "Input", PORT_OUT);    
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
    
    float dY = 0;
    float dX = 0;
    if (ports[0]->Y == sizeY)  /* if our port is at the bottom, we move the text by one */
	dX = 1.2;
    if (name != "") {
      if (selected && single && edit_mode) {
          struct timeval tv;
          gettimeofday(&tv, NULL);
          if (tv.tv_usec > 500000)
             canvas->draw_text(name + "|", X + dX, Y + sizeY +dY, sizeX, 1);
          else
             canvas->draw_text(name + " ", X + dX, Y + sizeY + dY, sizeX, 1);
      }
      else {
          canvas->draw_text(name + " ", X + dX, Y + sizeY + dY, sizeX, 1);
      }
    }
    if (ports[0]->value.type == VALUE_TYPE_INT) {
	char buf[128];
	std::string s;
	sprintf(buf, "%li", ports[0]->value.intval);
	s = buf;
	canvas->draw_text(s, X+ 0.6, Y + 0.6, 1.8,1.8);
    }

    for (auto port : ports) {
        port->drawAt(canvas, X,Y, COLOR_WIRE_SOLID);
        port->draw_wires(canvas);
    }
}



void model_toggle::update_value(struct value *newvalue, int ttl)
{
    if (!newvalue->valid)
      return;
    if (ttl <= 1)
      return;
    value = *newvalue;
    ports[0]->update_value(&value, ttl - 1);
    notify(ttl - 1);
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
    value.intval = value.boolval;
    
    update_value(&value, DEFAULT_TTL);
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


void model_toggle::handle_event(class canvas *canvas, SDL_Event &event)
{
    if (!selected || !single)
        return;
    switch (event.type) {
	case SDL_KEYDOWN:        
        	switch (event.key.keysym.sym) {
                case SDLK_RETURN:
                    edit_mode = !edit_mode;
                    break;
                }
                break;
        }
    if (edit_mode)
      labelevent(event, &name);
    ports[0]->update_name(name);
}

void model_toggle::rotate_ports(void)
{
    for (auto port : ports) {
        float x,y,_x,_y;
        x = port->X - (sizeX-1)/2.0;
        y = port->Y - (sizeY-1)/2.0;
        
        _x = y;
        _y = -x;
        
        port->X = _x + (sizeX-1)/2.0 ;
        port->Y = _y + (sizeY-1)/2.0;
        port->screenX = X + port->X;
        port->screenY = Y + port->Y;
        
        port->route_wires();
    }
    reseat();
}
