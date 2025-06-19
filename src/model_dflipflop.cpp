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
#include "model_dflipflop.h"
#include "port.h"
#include "contextmenu.h"
#include "name.h"

#include <sys/time.h>

model_dflipflop::model_dflipflop(float _X, float _Y)  : element(1, 1, "Flipflop")
{
    sizeX = 3;
    sizeY = 4;    
    X = floorf(_X);
    Y = floorf(_Y);
    
    add_port(-1, 1, "D", PORT_IN, 1);    
    add_port(-1, 3, "clk", PORT_IN, 1);    
    add_port(sizeX, 1, "Q", PORT_OUT, 1);    
    add_port(sizeX, 2, "ǭ", PORT_OUT, 1);    
    menu->add_item("Edit name", callback_editname);
    
    name_edit = new class name(&name);
}

model_dflipflop::~model_dflipflop(void)
{
     if (name_edit)
        delete name_edit;
}

void model_dflipflop::drawAt(class canvas *canvas, float X, float Y, int type)
{
    if (!selected)
      name_edit->set_edit_mode(false);
    if (selected) {
        canvas->draw_image_rotated("assets/dflipflop_selected.png", X, Y, sizeX, sizeY, Alpha(type), angle);
    } else {
        canvas->draw_image_rotated("assets/dflipflop.png", X, Y, sizeX, sizeY, Alpha(type), angle);
    }
    name_edit->drawAt(canvas, X, Y + sizeY, sizeX);

    for (auto port : ports) {
        port->drawAt(canvas, X,Y, COLOR_WIRE_SOLID);
        port->draw_wires(canvas);
    }
}



void model_dflipflop::to_json(json &j)
{
     element::to_json(j);
     j["value"] = value;
     j["previous_clock"] = previous_clock;
}
void model_dflipflop::from_json(json &j)
{
     element::from_json(j);
     value = j["value"];
     previous_clock = j.value("previous_clock", previous_clock);
}

void model_dflipflop::handle_event(class canvas *canvas, SDL_Event &event)
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

void model_dflipflop::calculate(int ttl)
{
     struct value newclock;
     newclock = ports[1]->value;
     
     if (newclock.boolval == previous_clock.boolval)
       return;
     if (newclock.boolval) { /* rising edge */
          bool changed = false;
          previous_clock = newclock;
          /* latch in the D value */
          
          if (memcmp(&value, &(ports[0]->value), sizeof(struct value)) != 0)
              changed = true;
          value = ports[0]->value;  
          
          if (changed)
              queue_calculate(this); /* schedule calculations for the propagation */
     };
     
     previous_clock = newclock;
}

void model_dflipflop::queued_calculate(int ttl)
{
     struct value notQ;

     update_value_net(&value, 2, ttl - 1);

     notQ = value;
     notQ.boolval = !notQ.boolval;
     update_value_net(&notQ, 3, ttl - 1);

}


std::string model_dflipflop::get_verilog_main(void)
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
    	

        s = s + ".Q_n(";
        s = s + ports[3]->get_net_verilog_name();
        s = s + ")";

    return s;
}

/* TODO -- rather than a pure basic SOP, run espresso on this and make it more minimal */
std::string model_dflipflop::get_verilog_modules(std::string path)
{
	std::string s = "";
	
	s = s + "module " + verilog_module_name + "\n";
	s = s + "(input D, input clk, output reg Q, output Q_n);\n\n";
	
	s = s + "always @(posedge clk) \n";
	s = s + "begin\n";
	s = s + "Q <= D;\n";
        s = s + "end \n";
        s = s + "assign Q_n = !Q;\n";
	s = s + "endmodule\n\n";
	
	
	return s;
}


