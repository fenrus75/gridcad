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
#include "net.h"
#include "port.h"
#include "wire.h"

#include <sys/time.h>


static void callback_toggle_datascope_2x(class element *element)
{
     auto *e = static_cast<class model_datascope *>(element);
     e->toggle_scale_2x();
}
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
    menu->add_item("2x size", callback_toggle_datascope_2x);
    menu->add_item("Enum", callback_enum);
    menu->add_item("Decimal", callback_decimal);
    menu->add_item("Hex", callback_hexadecimal);
    menu->add_item("Binary", callback_binary);
}

void model_datascope::set_scale_2x(bool enabled)
{
    if (scale_2x == enabled)
        return;

    const int old_sizeX = sizeX;
    const int old_sizeY = sizeY;
    scale_2x = enabled;
    if (scale_2x)
        menu->rename_item("2x size", "1x size");
    else
        menu->rename_item("1x size", "2x size");

    sizeX = scale_2x ? 8 : 4;
    sizeY = scale_2x ? 6 : 3;

    if (!ports.empty()) {
        if (ports[0]->X == -1) {
            ports[0]->X = -1;
            ports[0]->Y = sizeY / 2;
        } else if (ports[0]->Y == -1) {
            ports[0]->X = sizeX / 2;
            ports[0]->Y = -1;
        } else if (ports[0]->X == old_sizeX) {
            ports[0]->X = sizeX;
            ports[0]->Y = sizeY / 2;
        } else if (ports[0]->Y == old_sizeY) {
            ports[0]->X = sizeX / 2;
            ports[0]->Y = sizeY;
        }
    }

    reseat();
    reroute_all_wires();
}

static bool is_autogen_wire_name(std::string_view nm)
{
    return nm.empty() || nm.starts_with("Wire-") || nm.starts_with("wire_") || nm.starts_with("$");
}

static std::string extract_non_auto_wire_name(const class wire *w)
{
    if (!w)
        return "";
    if (!is_autogen_wire_name(w->name))
        return w->name;
    if (!w->get_vcd_name().empty()) {
        const std::string &vn = w->get_vcd_name();
        const size_t dot = vn.rfind('.');
        const std::string leaf = (dot != std::string::npos) ? vn.substr(dot + 1) : vn;
        if (!is_autogen_wire_name(leaf))
            return leaf;
    }
    return "";
}

std::string model_datascope::connected_wire_label(void) const
{
    if (ports.empty())
        return "";

    for (const auto &w : ports[0]->get_wires()) {
        std::string nm = extract_non_auto_wire_name(w.get());
        if (!nm.empty())
            return nm;
    }

    if (class net *n = ports[0]->get_net()) {
        for (const class wire *w : n->get_wires()) {
            std::string nm = extract_non_auto_wire_name(w);
            if (!nm.empty())
                return nm;
        }
    }

    return "";
}

class wire *model_datascope::connected_vcd_wire(void) const
{
    if (ports.empty())
        return nullptr;

    for (const auto &w : ports[0]->get_wires()) {
        if (w && !w->get_vcd_name().empty())
            return w.get();
    }

    if (class net *n = ports[0]->get_net()) {
        for (class wire *w : n->get_wires()) {
            if (w && !w->get_vcd_name().empty())
                return w;
        }
    }
    return nullptr;
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

    const float s = scale_2x ? 2.0f : 1.0f;
    const std::string wire_lbl = scale_2x ? connected_wire_label() : "";

    if (!ports.empty() && !ports[0]->value.qualifier_valid) {
        canvas->draw_box_rgba(X + 0.55f * s, Y + 0.55f * s,
                              X + 3.45f * s, Y + 2.45f * s,
                              80, 80, 80, 150);
        if (!wire_lbl.empty())
            canvas->draw_text(wire_lbl, X + 0.6f * s, Y + 0.55f * s, 2.8f * s, 0.48f * s);
    } else {
        const std::string val_text = format_display_text();
        if (!wire_lbl.empty()) {
            canvas->draw_text(wire_lbl, X + 0.6f * s, Y + 0.55f * s, 2.8f * s, 0.48f * s);
            if (!val_text.empty())
                canvas->draw_text(val_text, X + 0.6f * s, Y + 1.08f * s, 2.8f * s, 1.32f * s);
        } else if (!val_text.empty()) {
            canvas->draw_text(val_text, X + 0.6f * s, Y + 0.6f * s, 2.8f * s, 1.8f * s);
        }
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
     if (scale_2x)
         j["scale_2x"] = true;
}
void model_datascope::from_json(json &j)
{
     element::from_json(j);
     format = j.value("format", 3);
     const bool want_2x = j.value("scale_2x", false);
     if (want_2x != scale_2x) {
         scale_2x = want_2x;
         if (scale_2x)
             menu->rename_item("2x size", "1x size");
         else
             menu->rename_item("1x size", "2x size");
     }
     sizeX = scale_2x ? 8 : 4;
     sizeY = scale_2x ? 6 : 3;
}

void model_datascope::rotate_ports(void)
{
    const int mid_x = sizeX / 2;
    const int mid_y = sizeY / 2;
    if (ports[0]->X == -1) {
      ports[0]->X = mid_x;
      ports[0]->Y = -1;
    } else if (ports[0]->Y == -1) {
      ports[0]->X = sizeX;
      ports[0]->Y = mid_y;
    } else if (ports[0]->X == sizeX) {
      ports[0]->X = mid_x;
      ports[0]->Y = sizeY;
    } else {
      ports[0]->X = -1;
      ports[0]->Y = mid_y;
    }
    ports[0]->screenX = X + ports[0]->X;
    ports[0]->screenY = Y + ports[0]->Y;
    
    ports[0]->route_wires();
    reseat();
}
