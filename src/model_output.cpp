#include "gridcad.h"
#include "model_output.h"
#include "port.h"

#include <sys/time.h>

model_output::model_output(float _X, float _Y)  : element(1, 1, "Output")
{
    sizeX = 3;
    sizeY = 3;    
    X = floorf(_X);
    Y = floorf(_Y);
    
    add_port(-1, 1, "Output", PORT_IN);    
}

model_output::~model_output(void)
{
}

void model_output::drawAt(class canvas *canvas, float X, float Y, int type)
{
    if (selected) {
        canvas->draw_image("assets/output_base.png", X, Y, sizeX, sizeY, Alpha(type));
    } else if (ports[0]->value.boolval) {
        canvas->draw_image("assets/output_on.png", X, Y, sizeX, sizeY, Alpha(type));
    } else {	
        canvas->draw_image("assets/output_off.png", X, Y, sizeX, sizeY, Alpha(type));
    }
    float dX = 0.0;
    if (ports[0]->Y == sizeY)
	dX = 1.2;
    if (selected && single && edit_mode) {
          struct timeval tv;
          gettimeofday(&tv, NULL);
          if (tv.tv_usec > 500000)
             canvas->draw_text(name + "|", X + dX, Y + sizeY, sizeX, 1);
          else
             canvas->draw_text(name + " ", X + dX, Y + sizeY, sizeX, 1);
    } else {
      canvas->draw_text(name + " ", X + dX, Y + sizeY, sizeX, 1);
    }

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

void model_output::handle_event(SDL_Event &event)
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

struct value model_output::get_value(void)
{
    return ports[0]->value;
}

void model_output::rotate_ports(void)
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
