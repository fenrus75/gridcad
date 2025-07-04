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
#include "element.h"
#include "model_clock.h"
#include "port.h"

#include <sys/time.h>


void callback_stop_clock(class element *element)
{
	SDL_Event ev = {};


	ev.type = EVENT_STOP_CLOCK;
	ev.user.code = 0;

	SDL_PushEvent(&ev);
}

void callback_start_clock(class element *element)
{
	SDL_Event ev = {};


	ev.type = EVENT_START_CLOCK;
	ev.user.code = 0;

	SDL_PushEvent(&ev);
}
void callback_single_clock(class element *element)
{
	SDL_Event ev = {};


	ev.type = EVENT_SINGLE_CLOCK;
	ev.user.code = 0;

	SDL_PushEvent(&ev);
        run_queued_calculations();	
        return;
	/* we send 2 events, one for rising one for falling clock */
	ev.type = EVENT_SINGLE_CLOCK;
	ev.user.code = 0;

	SDL_PushEvent(&ev);
}

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

void set_timer(void)
{
    if (timer_set)
        return;
    timer_set = true;
    
    timer = SDL_AddTimer(500, SDL_timer_func, NULL);
}


model_clock::model_clock(float _X, float _Y)  : element(1, 1, "clk")
{
    sizeX = 3;
    sizeY = 3;    
    X = floorf(_X);
    Y = floorf(_Y);
    
    set_timer();
    
    global_clock.valid = true;
    
    add_port(1, sizeY, "clk", PORT_OUT, 1);    
    ports[0]->allow_fancy_verilog();
    menu->add_item("Stop clock", callback_stop_clock);
    menu->add_item("Start clock", callback_start_clock);
    menu->add_item("Single clock", callback_single_clock);
}

model_clock::~model_clock(void)
{
}

void model_clock::drawAt(class basecanvas *canvas, float X, float Y, int type)
{

    if (selected) {
        canvas->draw_image("assets/clock_selected.png", X, Y, sizeX, sizeY, Alpha(type));
    } else {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        float f;
        f = tv.tv_usec / 1000000.0;
        f = f * (450-25);
        
        if (!clock_running)
          f = 0;
        canvas->draw_image("assets/clock_off.png", X, Y, sizeX, sizeY, Alpha(type));
        canvas->draw_image_fragment("assets/clock_wave.png", X+0.6, Y+0.6, sizeX-1.2, sizeY-1.2, 25 + f, 0, 450 -25, 189 );
    }

    for (auto port : ports) {
        port->drawAt(canvas, X,Y, COLOR_WIRE_SOLID);
        port->draw_wires(canvas);
    }
}



void model_clock::to_json(json &j)
{
     element::to_json(j);
     j["clock_running"] = clock_running;
}
void model_clock::from_json(json &j)
{
     element::from_json(j);
     clock_running = j.value("clock_running", true);
     ports[0]->allow_fancy_verilog();
}

void model_clock::handle_event(class basecanvas *canvas, SDL_Event &event)
{
    if (event.type == SDL_timer_event) {
	    update_value_net(&global_clock, 0, DEFAULT_TTL);
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

std::string model_clock::get_verilog_main(void)
{
    std::string s = "";
    
    if (!ports[0]->has_net())
      return "";
    
    if (get_verilog_name() != ports[0]->get_net_verilog_name())
        s = s + "assign " + ports[0]->get_net_verilog_name() + " = " + get_verilog_name() + ";\n";
    
    return s;
}

bool model_clock::mouse_select(float _X, float _Y)
{
    /* convert _X and _Y to be relative to the center of the model */
    _X = _X - X - sizeX/2.0;
    _Y = _Y - Y - sizeY/2.0;
    
    if ( (_X * _X) + (_Y * _Y) > 1.0)  /* click is outside center button */
         return false;
         
    if (clock_running)
         return false;
         
    callback_single_clock(NULL);
         
    return true;
}


struct value global_clock = {};

void model_clock::create_verilog_name(int seqno, std::vector<std::string> *existing)
{
    element::create_verilog_name(seqno, existing);
    ports[0]->update_fancy_name(verilog_name);
}
