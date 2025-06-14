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
#include "model_memory.h"
#include "port.h"
#include "contextmenu.h"
#include "name.h"

#include <algorithm>
#include <string>

#include <sys/time.h>


void callback_256(class element *element)
{
    class model_memory *mem = (class model_memory *) element;
    mem->update_memory_size(256);
}

void callback_512(class element *element)
{
    class model_memory *mem = (class model_memory *) element;
    mem->update_memory_size(512);
}

void callback_1024(class element *element)
{
    class model_memory *mem = (class model_memory *) element;
    mem->update_memory_size(1024);
}

void callback_2048(class element *element)
{
    class model_memory *mem = (class model_memory *) element;
    mem->update_memory_size(2048);
}

model_memory::model_memory(float _X, float _Y)  : element(1, 1, "Memory")
{
    sizeX = 6;
    sizeY = 4;    
    X = floorf(_X);
    Y = floorf(_Y);

    data.resize(512);
    
    add_port(-1, 3, "clk", PORT_IN);    
    add_port(sizeX, 3, "Wr", PORT_IN, 1);
    add_port(1, sizeY, "Addr", PORT_IN, highest_addr_bit(data.size())+1);
    add_port(sizeX, 2, "DI", PORT_IN, 8);    
    add_port(4, sizeY, "DO", PORT_OUT, 8);    
    menu->add_item("Edit name", callback_editname);
    menu->add_item("Set 256 byte size", callback_256);
    menu->add_item("Set 512 byte size", callback_512);
    menu->add_item("Set 1024 byte size", callback_1024);
    menu->add_item("Set 2048 byte size", callback_2048);
    
    name_edit = new class name(&name);
}

model_memory::~model_memory(void)
{
     if (name_edit)
        delete name_edit;
}

void model_memory::drawAt(class canvas *canvas, float X, float Y, int type)
{
    if (!selected)
      name_edit->set_edit_mode(false);
    if (selected) {
        canvas->draw_image("assets/memory_selected.png", X, Y, sizeX, sizeY, Alpha(type));
    } else {
        canvas->draw_image("assets/memory.png", X, Y, sizeX, sizeY, Alpha(type));
    }
    name_edit->drawAt(canvas, X, Y - 1, sizeX);

    hover_ports(canvas);
    for (auto port : ports) {
        port->drawAt(canvas, X,Y, COLOR_WIRE_SOLID);
        port->draw_wires(canvas);
    }
}



void model_memory::to_json(json &j)
{
     element::to_json(j);
     j["previous_clock"] = previous_clock;
     j["data"] = data;
     j["previous_address"] = previous_address;
}
void model_memory::from_json(json &j)
{
     element::from_json(j);
     previous_clock = j.value("previous_clock", previous_clock);
     previous_address = j.value("previous_address", 0);
     data = j.value("data", data);
}

void model_memory::handle_event(class canvas *canvas, SDL_Event &event)
{
    element::handle_event(canvas, event);
    
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

void model_memory::calculate(int ttl)
{
     struct value newclock;
     newclock = ports[0]->value;
     
     if (newclock.boolval == previous_clock.boolval)
       return;

     if (newclock.boolval) { /* rising edge */
          previous_clock = newclock;
          
          previous_address = ports[2]->value.intval;
          
          if (ports[1]->value.boolval) { /* write enable */
              if (previous_address < data.size()) {
                  data[previous_address] = ports[3]->value.intval;
              }
          }
          
          /* TODO: latch in value on write_enable */
          /* TODO: latch in the address */
          
          queue_calculate(this); /* schedule calculations for the propagation */
     };
     
     previous_clock = newclock;
}

void model_memory::queued_calculate(int ttl)
{
     struct value val = {};
     
     val.valid = true;
     val.type = VALUE_TYPE_INT;
     val.intval = 0;
     if (previous_address < data.size())
         val.intval = data[previous_address];
         
     ports[4]->update_value(&val, ttl - 1); 
}


std::string model_memory::get_verilog_main(void)
{
    unsigned int addrbits = highest_addr_bit(data.size());
    std::string s = "";
    
    std::vector<std::string> wiremap;
    if (verilog_module_name == "")
	    verilog_module_name = append_random_bits(verilog_name + "_tt_");

    s = s + verilog_module_name + " " + get_verilog_name() + "(";
    bool first = true;

        ports[0]->collect_wires(&wiremap);
        wiremap.push_back("");
        if (wiremap.size() != 0) {

    	    if (!first)
  	        s = s + ", ";
            first = false;
    
            s = s + ".clk(";
            s = s + wiremap[0];
            s = s + ")";
    	
            wiremap.clear();
        }

        ports[1]->collect_wires(&wiremap);
        wiremap.push_back("");
        if (wiremap.size() != 0) {

    	    if (!first)
  	        s = s + ", ";
            first = false;
    
            s = s + ".WrEn(";
            s = s + wiremap[0];
            s = s + ")";
    	
            wiremap.clear();
        }


        ports[2]->collect_wires(&wiremap);
        wiremap.push_back("");
        if (wiremap.size() != 0) {

    	    if (!first)
  	        s = s + ", ";
            first = false;
    
            s = s + ".Addr(";
            s = s + wiremap[0] + "[" + std::to_string(addrbits) + ":0]";
            s = s + ")";
    	
            wiremap.clear();
        }


        ports[3]->collect_wires(&wiremap);
        wiremap.push_back("");
        if (wiremap.size() != 0) {

    	    if (!first)
  	        s = s + ", ";
            first = false;
    
            s = s + ".Di(";
            s = s + wiremap[0];
            s = s + ")";
    	
            wiremap.clear();
        }
        ports[4]->collect_wires(&wiremap);
        wiremap.push_back("");
        if (wiremap.size() != 0) {

    	    if (!first)
  	        s = s + ", ";
            first = false;
    
            s = s + ".Do(";
            s = s + wiremap[0];
            s = s + ")";
    	
            wiremap.clear();
        }

    s = s + ");\n";
    
    return s;
}

/* TODO -- rather than a pure basic SOP, run espresso on this and make it more minimal */
std::string model_memory::get_verilog_modules(std::string path)
{
        unsigned int addrbits = highest_addr_bit(data.size());
        
	std::string s = "";
	s = s + "module " + verilog_module_name + "\n";
	s = s + "(input [7:0] Di, input clk, output reg [7:0] Do, input [" + std::to_string(addrbits) +":0] Addr, input WrEn);\n\n";

#if 0
    initial begin
        $readmemh("memory.txt", memory); // Load from a text file
    end
#endif

        s = s + "reg [7:0] data [0:" + std::to_string(data.size()-1) + "];";
	s = s + "always @(posedge clk) \n";
	s = s + "begin\n";
	s = s + "  if (WrEn) begin\n";
	s = s + "      data[Addr["+ std::to_string(addrbits) + ":0]] <= Di;\n";
	s = s + "  end;\n";
	s = s + "  Do <= data[Addr["+std::to_string(addrbits) + ":0]];\n";
        s = s + "end \n";

	s = s + "endmodule\n\n";
	
	return s;
}



unsigned int model_memory::highest_addr_bit(unsigned int size)
{
    unsigned int i,j;
    i = 0;
    j = 2;
    while (1) {
        if (size <= j) {
            return i;
        }
        j = j << 1;
        i ++;
    }
    return 0;
}

void model_memory::update_memory_size(unsigned int newsize)
{
    data.resize(newsize);
    ports[2]->set_width(highest_addr_bit(data.size())+1);
}