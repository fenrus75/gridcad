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
#include "model_datascope.h"
#include "port.h"
#include "wire.h"

#include <sys/time.h>


void callback_decimal(class element *element)
{
     class model_datascope *e = (class model_datascope *)element;
     e->set_format(0);
}
void callback_hexadecimal(class element *element)
{
     class model_datascope *e = (class model_datascope *)element;
     e->set_format(1);
}
void callback_binary(class element *element)
{
     class model_datascope *e = (class model_datascope *)element;
     e->set_format(2);
}
void callback_enum(class element *element)
{
     class model_datascope *e = (class model_datascope *)element;
     e->set_format(3);
}

model_datascope::model_datascope(float _X, float _Y)  : element(1, 1, "Output")
{
    sizeX = 4;
    sizeY = 3;    
    X = floorf(_X);
    Y = floorf(_Y);
    
    add_port(-1, 1, "Output", PORT_IN);    
    menu->add_item("Enum", callback_enum);
    menu->add_item("Decimal", callback_decimal);
    menu->add_item("Hex", callback_hexadecimal);
    menu->add_item("Binary", callback_binary);
}

std::string model_datascope::format_display_text(void) const
{
    if (ports.empty())
        return "";
    if (ports[0]->value.type != VALUE_TYPE_INT && ports[0]->value.type != VALUE_TYPE_BOOL)
        return "";
    if (!ports[0]->value.qualifier_valid)
        return "---";

    const uint64_t raw_val = (ports[0]->value.type == VALUE_TYPE_BOOL)
                                 ? (ports[0]->value.boolval ? 1ULL : 0ULL)
                                 : ports[0]->value.intval;
    if (format == 3) {
        std::string lbl = ports[0]->get_label(raw_val);
        if (!lbl.empty())
            return lbl;
        return std::format("{}", raw_val);
    }
    if (format == 1)
        return std::format("x{:x}", raw_val);
    if (format == 2)
        return std::format("{:b}", raw_val);
    return std::format("{}", raw_val);
}

void model_datascope::drawAt(class basecanvas *canvas, float X, float Y, int type)
{
    if (selected) {
        canvas->draw_image("assets/datascope_selected.png", X, Y, sizeX, sizeY, Alpha(type));
    } else {
        canvas->draw_image("assets/datascope.png", X, Y, sizeX, sizeY, Alpha(type));
    }
    if (!ports.empty() && !ports[0]->value.qualifier_valid) {
        canvas->draw_box_rgba(X + 0.55f, Y + 0.55f, X + 3.45f, Y + 2.45f, 80, 80, 80, 150);
    } else {
        const std::string s = format_display_text();
        if (!s.empty())
            canvas->draw_text(s, X + 0.6, Y + 0.6, 2.8, 1.8);
    }

    for (auto &port : ports) {
        port->drawAt(canvas, X,Y, COLOR_WIRE_SOLID);
        port->draw_wires(canvas);
    }
}



void model_datascope::to_json(json &j)
{
     element::to_json(j);
     j["format"] = format;
}
void model_datascope::from_json(json &j)
{
     element::from_json(j);
     format = j.value("format", 3);
}

void model_datascope::rotate_ports(void)
{
        
    if (ports[0]->X == -1) {
      ports[0]->X = 1;
      ports[0]->Y = -1;
    } else if (ports[0]->Y == -1) {
      ports[0]->X = sizeX;
      ports[0]->Y = 1;
    } else if (ports[0]->X == sizeX) {
      ports[0]->X = 1;
      ports[0]->Y = sizeY;
    } else {
      ports[0]->X = -1;
      ports[0]->Y = 1;;
    }
    ports[0]->screenX = X + ports[0]->X;
    ports[0]->screenY = Y + ports[0]->Y;
    
    ports[0]->route_wires();
    reseat();
}
