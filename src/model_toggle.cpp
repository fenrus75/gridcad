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
#include "model_toggle.h"
#include "port.h"
#include "contextmenu.h"
#include "name.h"

#include <sys/time.h>


static void callback_wire1(class element *element)
{
  class model_toggle *toggle = (class model_toggle *) element;
  toggle->set_width(1);  
}
static void callback_bus4(class element *element)
{
  class model_toggle *toggle = (class model_toggle *) element;
  toggle->set_width(4);  
}

static void callback_bus8(class element *element)
{
  class model_toggle *toggle = (class model_toggle *) element;
  toggle->set_width(8);  
}

model_toggle::model_toggle(float _X, float _Y)  : element(1, 1, "Input")
{
    sizeX = 3;
    sizeY = 3;    
    X = floorf(_X);
    Y = floorf(_Y);
    value = {};
    value.valid = true;
    
    name_edit = new class name(&name);
    value_edit = new class name(&strvalue);
    value_edit->want_numbers_only();
    
    add_port(sizeX, 1, "Input", PORT_OUT);    
    
    ports[0]->allow_fancy_verilog();
    menu->add_item("Edit name", callback_editname);
    menu->add_item("Set as single wire", callback_wire1);
    menu->add_item("Set as 4 bit bus", callback_bus4);
    menu->add_item("Set as 8 bit bus", callback_bus8);
    
}

model_toggle::~model_toggle(void)
{
  delete name_edit;
  delete value_edit;
}

void model_toggle::drawAt(class basecanvas *canvas, float X, float Y, int type)
{
    std::string icon = "";
    
    if (!selected)
      name_edit->set_edit_mode(false);
      
    if (value.boolval) {
      icon = "assets/toggle_on.png";
    } else {	
      icon = "assets/toggle_off.png";
    }
    
    if (ports[0]->get_width() > 1)
      icon = "assets/toggle_bus.png";

    if (selected) {
        icon = "assets/toggle_base.png";
    }    
    
    canvas->draw_image(icon, X, Y, sizeX, sizeY, Alpha(type));
    
    if (ports[0]->Y == sizeY)  /* if our port is at the bottom, we move the text by one */
      name_edit->drawAt(canvas,X, Y - 1, sizeX);
    else
      name_edit->drawAt(canvas,X, Y + sizeY, sizeX);

    if (ports[0]->get_width() > 1 || value.type == VALUE_TYPE_INT) {
	char buf[128];
	sprintf(buf, "%li", ports[0]->value.intval);
	strvalue = buf;
//	canvas->draw_text(s, X+ 0.6, Y + 0.6, 1.8,1.8);
	value_edit->drawAt(canvas, X + 0.6, Y + 1, 1.8);
    }

    for (auto port : ports) {
        port->drawAt(canvas, X,Y, COLOR_WIRE_SOLID);
        port->draw_wires(canvas);
    }
}



void model_toggle::update_value(struct value *newvalue, int ttl)
{
    char buffer[128];
    if (!newvalue->valid)
      return;
    if (ttl <= 1)
      return;
    value = *newvalue;

    update_value_net(&value, 0, ttl - 1);
    notify(ttl - 1);
    sprintf(buffer, "%lu", value.intval);
    strvalue = buffer;
}

bool model_toggle::mouse_select(float _X, float _Y)
{
    /* convert _X and _Y to be relative to the center of the model */
    _X = _X - X - sizeX/2.0;
    _Y = _Y - Y - sizeY/2.0;
    
    if ( (_X * _X) + (_Y * _Y) > 1.0)  /* click is outside center button */
         return false;
         
    if (ports[0]->get_width() <= 1) {    
      value.boolval = !value.boolval;
      value.valid = true;
      value.is_clock = false;
      value.intval = value.boolval;
    } else {
      value.type = VALUE_TYPE_INT;
      value_edit->toggle_edit_mode();
    }
    
    
    update_value(&value, DEFAULT_TTL);
    run_queued_calculations();
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
     ports[0]->allow_fancy_verilog();
}


void model_toggle::handle_event(class basecanvas *canvas, SDL_Event &event)
{
    if (value_edit->get_edit_mode()) {
      char buffer[128];
      sprintf(buffer, "%lu", value.intval);
      strvalue = buffer;
      value_edit->handle_event(event);
      if (strvalue != "")
        value.intval = std::stoi(strvalue);
      else 
        value.intval = 0;
      update_value(&value, DEFAULT_TTL);
//      printf("strvalue is %s \n", strvalue.c_str());
    }
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

std::string model_toggle::get_verilog_main(void)
{
    std::string s = "";
    
    if (!ports[0]->has_net())
        return "";

    if (get_verilog_name() != ports[0]->get_net_verilog_name())
      s = s + "assign " + ports[0]->get_net_verilog_name() + " = " + get_verilog_name() + ";\n";

    return s;
}

std::string model_toggle::get_verilog_width(void)
{
    std::string s = "";
    unsigned int w = ports[0]->get_width();
    
    if (w > 1)
      s = "[" + std::to_string(w-1) + ":0]";
    return s;
}

void model_toggle::create_verilog_name(int seqno, std::vector<std::string> *existing)
{
    element::create_verilog_name(seqno, existing);
    ports[0]->update_fancy_name(verilog_name);
}
