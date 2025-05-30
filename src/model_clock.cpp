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
#include "model_clock.h"
#include "port.h"

#include <sys/time.h>


static bool timer_set = false;

Uint32 SDL_timer_func(Uint32 interval, void *param)
{
     SDL_Event ev = {};


    ev.type = SDL_timer_event;
    ev.user.code = 0;
    ev.user.data1 = NULL;
    ev.user.data2 = NULL;

    if (!SDL_PushEvent(&ev)) {
     printf("Push failure %s\n", SDL_GetError());
    }
    return interval;;
}

static SDL_TimerID timer;

static void set_timer(void)
{
    if (timer_set)
        return;
    timer_set = true;
    
    timer = SDL_AddTimer(500, SDL_timer_func, NULL);
}


model_clock::model_clock(float _X, float _Y)  : element(1, 1, "Output")
{
    sizeX = 3;
    sizeY = 3;    
    X = floorf(_X);
    Y = floorf(_Y);
    
    set_timer();
    
    add_port(-1, 1, "clk", PORT_OUT);    
}

model_clock::~model_clock(void)
{
}

void model_clock::drawAt(class canvas *canvas, float X, float Y, int type)
{
    if (selected) {
        canvas->draw_image("assets/clock_selected.png", X, Y, sizeX, sizeY, Alpha(type));
    } else {
        if (value.boolval) {
            canvas->draw_image("assets/clock_on.png", X, Y, sizeX, sizeY, Alpha(type));
        } else {	
            canvas->draw_image("assets/clock_off.png", X, Y, sizeX, sizeY, Alpha(type));
        }
    }

    for (auto port : ports) {
        port->drawAt(canvas, X,Y, COLOR_WIRE_SOLID);
        port->draw_wires(canvas);
    }
}



void model_clock::to_json(json &j)
{
     element::to_json(j);
     j["value"] = value;
}
void model_clock::from_json(json &j)
{
     element::from_json(j);
     value = j.value("value", value);
}

void model_clock::handle_event(class canvas *canvas, SDL_Event &event)
{
    value.valid = true;
    if (event.type == SDL_timer_event) {
	    value.boolval = !value.boolval;
	    ports[0]->update_value(&value, DEFAULT_TTL);
    }
}

void model_clock::rotate_ports(void)
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


struct value model_clock::get_value(void)
{
    return ports[0]->value;
}



