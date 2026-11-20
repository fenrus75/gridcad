/*
 * gridcad -- a tool to do digital circuit design with a GUI
 *
 * (C) Copyright 2025  -- Arjan van de Ven
 *
 * Licensed under the terms of the
 * GNU General Public LIcense version 3.0
 */

#include "gridcad.h"
#include "model_eq.h"
#include "port.h"

#include <algorithm>
#include <format>

static uint64_t width_mask(int width)
{
    if (width <= 0)
        return 0;
    if (width >= 64)
        return ~uint64_t{0};
    return (uint64_t{1} << width) - 1;
}

static uint64_t read_u64(const struct value &v, int width)
{
    uint64_t raw = (v.type == VALUE_TYPE_BOOL) ? (v.boolval ? 1ULL : 0ULL) : v.intval;
    return raw & width_mask(width);
}

model_eq::model_eq(float _X, float _Y, int width)
    : element(3, 3, "eq")
{
    sizeX = 3;
    sizeY = 3;
    X = floorf(_X);
    Y = floorf(_Y);

    bus_width = std::clamp(width, 1, 64);

    add_port(-1, 0, "a", PORT_IN, bus_width);
    add_port(-1, 2, "b", PORT_IN, bus_width);
    add_port(sizeX, 1, "y", PORT_OUT, 1);
}

void model_eq::set_bus_width(int width)
{
    bus_width = std::clamp(width, 1, 64);
    ports[0]->set_width(bus_width);
    ports[1]->set_width(bus_width);
}

void model_eq::calculate(int ttl)
{
    bool eq = read_u64(ports[0]->value, bus_width) == read_u64(ports[1]->value, bus_width);

    struct value out = {};
    out.valid = true;
    out.type = VALUE_TYPE_BOOL;
    out.boolval = eq;
    out.intval = eq ? 1 : 0;

    update_value_net(&out, 2, ttl - 1);
}

void model_eq::drawAt(class basecanvas *canvas, float X, float Y, int type)
{
    canvas->draw_image_rotated(selected ? "assets/buttonbox_hover.png" : "assets/buttonbox.png",
                               X, Y, sizeX, sizeY, Alpha(type), angle);
    canvas->draw_text("==", X + 0.2f, Y + 0.8f, sizeX - 0.4f, 1.4f);

    for (auto &port : ports) {
        port->drawAt(canvas, X, Y, COLOR_WIRE_SOLID);
        port->draw_wires(canvas);
    }
}

void model_eq::to_json(json &j)
{
    element::to_json(j);
    j["bus_width"] = bus_width;
}

void model_eq::from_json(json &j)
{
    element::from_json(j);
    set_bus_width(j.value("bus_width", 8));
}

std::string model_eq::get_verilog_main(void)
{
    if (!ports[2]->has_net())
        return "";
    return "assign " + ports[2]->get_net_verilog_name() + " = ("
         + ports[0]->get_net_verilog_name("0") + " == "
         + ports[1]->get_net_verilog_name("0") + ");\n";
}
