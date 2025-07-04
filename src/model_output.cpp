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
#include "model_output.h"
#include "port.h"
#include "contextmenu.h"

#include <sys/time.h>


static void callback_wire1(class element *element)
{
  class model_output *output = (class model_output *) element;
  output->set_width(1);  
}
static void callback_bus4(class element *element)
{
  class model_output *output = (class model_output *) element;
  output->set_width(4);  
}

static void callback_bus8(class element *element)
{
  class model_output *output = (class model_output *) element;
  output->set_width(8);  
}
static void callback_bus16(class element *element)
{
  class model_output *output = (class model_output *) element;
  output->set_width(16);  
}
static void callback_bus32(class element *element)
{
  class model_output *output = (class model_output *) element;
  output->set_width(32);  
}


model_output::model_output(float _X, float _Y)  : element(1, 1, "Output")
{
    sizeX = 3;
    sizeY = 3;    
    X = floorf(_X);
    Y = floorf(_Y);

    name_edit = new class name(&name);
    
    add_port(-1, 1, "Output", PORT_IN);    
    ports[0]->allow_fancy_verilog();
    menu->add_item("Edit name", callback_editname);
    menu->add_item("Set as single wire", callback_wire1);
    menu->add_item("Set as 4 bit bus", callback_bus4);
    menu->add_item("Set as 8 bit bus", callback_bus8);
    menu->add_item("Set as 16 bit bus", callback_bus16);
    menu->add_item("Set as 32 bit bus", callback_bus32);
}

model_output::~model_output(void)
{
    delete name_edit;
}

void model_output::drawAt(class basecanvas *canvas, float X, float Y, int type)
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
    if (ports[0]->Y == sizeY)
      name_edit->drawAt(canvas, X, Y - 1, sizeX);
    else
      name_edit->drawAt(canvas, X, Y + sizeY, sizeX);

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
     ports[0]->allow_fancy_verilog();
}

void model_output::handle_event(class basecanvas *canvas, SDL_Event &event)
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

std::string model_output::get_verilog_main(void)
{
    std::string p = "";
    std::string s = "";
    p = std::to_string(ports[0]->get_width()) + "'b0";
    if (get_verilog_name() != ports[0]->get_net_verilog_name(p))
        s = s + "assign " + get_verilog_name() + " = " + ports[0]->get_net_verilog_name(p) + ";\n";
    return s;
}

std::string model_output::get_verilog_width(void)
{
    std::string s = "";
    unsigned int w = ports[0]->get_width();
    
    if (w > 1)
      s = "[" + std::to_string(w-1) + ":0]";
    return s;
}

void model_output::create_verilog_name(int seqno, std::vector<std::string> *existing)
{
    element::create_verilog_name(seqno, existing);
    ports[0]->update_fancy_name(verilog_name);
}
