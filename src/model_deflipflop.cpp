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

#include "model_deflipflop.h"
#include "port.h"
#include "contextmenu.h"
#include "name.h"

#include <sys/time.h>

model_deflipflop::model_deflipflop(float _X, float _Y)  : element(1, 1, "Flipflop")
{
    sizeX = 3;
    sizeY = 4;    
    X = floorf(_X);
    Y = floorf(_Y);
    
    add_port(-1, 1, "D", PORT_IN, 1);    
    add_port(-1, 3, "clk", PORT_IN, 1);    
    add_port(sizeX, 1, "Q", PORT_OUT, 1);    
    add_port(1, -1, "En", PORT_IN ,1);    
    menu->add_item("Edit name", callback_editname);
    
    name_edit = new class name(&name);
}

model_deflipflop::~model_deflipflop(void)
{
     if (name_edit)
        delete name_edit;
}

void model_deflipflop::drawAt(class basecanvas *canvas, float X, float Y, int type)
{
    if (!selected)
      name_edit->set_edit_mode(false);
    if (selected) {
        canvas->draw_image_rotated("assets/deflipflop_selected.png", X, Y, sizeX, sizeY, Alpha(type), angle);
    } else {
        canvas->draw_image_rotated("assets/deflipflop.png", X, Y, sizeX, sizeY, Alpha(type), angle);
    }
    name_edit->drawAt(canvas, X, Y + sizeY, sizeX);

    for (auto port : ports) {
        port->drawAt(canvas, X,Y, COLOR_WIRE_SOLID);
        port->draw_wires(canvas);
    }
}



void model_deflipflop::to_json(json &j)
{
     element::to_json(j);
     j["value"] = value;
     j["previous_clock"] = previous_clock;
}
void model_deflipflop::from_json(json &j)
{
     element::from_json(j);
     value = j["value"];
     previous_clock = j.value("previous_clock", previous_clock);
}

void model_deflipflop::handle_event(class basecanvas *canvas, SDL_Event &event)
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
}

void model_deflipflop::calculate(int ttl)
{
     struct value newclock;
     newclock = ports[1]->value;
     
     if (newclock.boolval == previous_clock.boolval)
       return;
     if (newclock.boolval) { /* rising edge */
          bool changed = false;
          previous_clock = newclock;
          /* latch in the D value -- but only if enable is set*/
          
           
          if (ports[3]->value.boolval) {
          
              if (memcmp(&value, &(ports[0]->value), sizeof(struct value)) != 0)
                  changed = true;
              value = ports[0]->value;  
          }
          if (changed)
              queue_calculate(this); /* schedule calculations for the propagation */
     };
     
     previous_clock = newclock;
}

void model_deflipflop::queued_calculate(int ttl)
{
     update_value_net(&value, 2, ttl - 1);
}


std::string model_deflipflop::get_verilog_main(void)
{
    std::string s = "";
    if (verilog_module_name == "")
	    verilog_module_name = append_random_bits(verilog_name + "_tt_");

    s = s + verilog_module_name + " " + get_verilog_name() + "(";
    
            s = s + ".D(";
            s = s + ports[0]->get_net_verilog_name();
            s = s + "),";
    	
            s = s + ".clk(";
            s = s + ports[1]->get_net_verilog_name();
            s = s + "),";
    	
            s = s + ".Q(";
            s = s + ports[2]->get_net_verilog_name();
            s = s + "),";
    	
            s = s + ".En(";
            s = s + ports[3]->get_net_verilog_name();
            s = s + ")";
    	
    s = s + ");\n";
    
    return s;
}

/* TODO -- rather than a pure basic SOP, run espresso on this and make it more minimal */
std::string model_deflipflop::get_verilog_modules(std::string path)
{
	std::string s = "";
	
	s = s + "module " + verilog_module_name + "\n";
	s = s + "(input D, input clk, output reg Q, input En);\n\n";
	
	s = s + "always @(posedge clk) \n";
	s = s + "begin\n";
	s = s + "  if (En) begin \n";
	s = s + "       Q <= D;\n";
	s = s + "  end\n";
        s = s + "end \n";
	s = s + "endmodule\n\n";
	
	
	return s;
}


