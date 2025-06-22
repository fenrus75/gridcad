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
#include "connector.h"
#include "port.h"
#include "contextmenu.h"

static void splice_wire_callback(class element *element)
{
    class connector *connector = (class connector *)element;
    
    connector->unsplice();
}


connector::connector(float _X, float _Y)  : element(3, 3, "")
{
    sizeX = 1;
    sizeY = 1;    
    X = floorf(_X);
    Y = floorf(_Y);

    add_port(0, 0, "Connector", PORT_INOUT);    
    ports[0]->is_connector = true;
    reseat();;
    menu->add_item("Join wire", splice_wire_callback);
}

connector::~connector(void)
{
}

void connector::drawAt(class basecanvas *canvas, float X, float Y, int type)
{
    std::string icon = "";
    for (auto port: ports) 
        port->draw_wires(canvas);
        
    if (ports[0]->value.boolval)
        icon = "assets/connector_green.png";
    else
        icon = "assets/connector_red.png";
        
    if (ports[0]->get_width() > 1)
        icon = "assets/connector_bus.png";
        
    canvas->draw_image(icon, X,Y,1,1);
    ports[0]->update_distances();

}

void connector::draw_phase2(class basecanvas *canvas, int type)
{
    std::string icon = "";
        
    if (ports[0]->value.boolval)
        icon = "assets/connector_green.png";
    else
        icon = "assets/connector_red.png";
        
    if (ports[0]->get_width() > 1)
        icon = "assets/connector_bus.png";
        
    canvas->draw_image(icon, X,Y,1,1);
}

void connector::fill_grid(class wiregrid *grid)
{
    grid->block_point(X, Y);
#if 0
    grid->add_soft_cost(X + 1, Y + 1, 0.3);
    grid->add_soft_cost(X + 1, Y - 1, 0.3);
    grid->add_soft_cost(X - 1, Y + 1, 0.3);
    grid->add_soft_cost(X - 1, Y - 1, 0.3);
#endif
}


bool connector::want_deleted(void)
{
    if (ports.size() == 0 || !ports[0]->has_wires())
        return true;
    return false;
}

void connector::unsplice(void)
{
    ports[0]->unsplice();
}

std::string connector::get_verilog_main(void)
{
  return "";
}
