/*
 * gridcad -- a tool to do digital circuit design with a GUI
 *
 * (C) Copyright 2025  -- Arjan van de Ven
 *
 * Licensed under the terms of the
 * GNU General Public LIcense version 3.0
 *
 * Authors:
 *    Arjan van de Ven <arjanvandeven@gmail.com>
 */

#include "gridcad.h"
#include "model_output.h"
#include "port.h"
#include "contextmenu.h"

#include <sys/time.h>

model_output::model_output(float _X, float _Y)  : element(1, 1, "Output")
{
    sizeX = 3;
    sizeY = 3;    
    X = floorf(_X);
    Y = floorf(_Y);

    name_edit = new class name(&name);
    
    add_port(-1, 1, "Output", PORT_IN);    
    menu->add_item("Edit name", callback_editname);
}

model_output::~model_output(void)
{
    delete name_edit;
}

void model_output::drawAt(class canvas *canvas, float X, float Y, int type)
{
    if (!selected)
      name_edit->set_edit_mode(false);
    if (selected) {
        canvas->draw_image("assets/output_base.png", X, Y, sizeX, sizeY, Alpha(type));
    } else {
        if (ports[0]->get_width() <= 1 && ports[0]->value.boolval) {
            canvas->draw_image("assets/output_on.png", X, Y, sizeX, sizeY, Alpha(type));
        } else if (ports[0]->get_width() <= 1) {	
            canvas->draw_image("assets/output_off.png", X, Y, sizeX, sizeY, Alpha(type));
        } else {
            canvas->draw_image("assets/output_bus.png", X, Y, sizeX, sizeY, Alpha(type));
        }
        
    }
    float dX = 0.0;
    if (ports[0]->Y == sizeY)
	dX = 1.8;
    name_edit->drawAt(canvas, X + dX, Y + sizeY, sizeX);

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



void model_output::to_json(json &j)
{
     element::to_json(j);
     j["value"] = value;
}
void model_output::from_json(json &j)
{
     element::from_json(j);
     value = j.value("value", value);
}

void model_output::handle_event(class canvas *canvas, SDL_Event &event)
{
    if (!selected || !single)
        return;
    switch (event.type) {
	case SDL_KEYDOWN:        
        	switch (event.key.keysym.sym) {
                case SDLK_RETURN:
                    name_edit->toggle_edit_mode();
                    break;
                }
                break;
        }
    name_edit->handle_event(event);
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
