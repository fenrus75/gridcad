/*
 * gridcad -- a tool to do digital circuit design with a GUI
 *
 * (C) Copyright 2025  -- Arjan van de Ven
 *
 * Licensed under the terms of the
 * GNU General Public LIcense version 3.0
 */

#include "gridcad.h"
#include "model_const.h"
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

model_const::model_const(float _X, float _Y, int width, uint64_t val)
    : element(2, 1, "const")
{
    sizeX = 2;
    sizeY = 1;
    X = floorf(_X);
    Y = floorf(_Y);

    bus_width = std::clamp(width, 1, 64);
    const_value = val & width_mask(bus_width);

    add_port(sizeX, 0, "out", PORT_OUT, bus_width);
    calculate(DEFAULT_TTL);
}

void model_const::set_const(int width, uint64_t val)
{
    bus_width = std::clamp(width, 1, 64);
    const_value = val & width_mask(bus_width);
    if (!ports.empty())
        ports[0]->set_width(bus_width);
    calculate(DEFAULT_TTL);
}

void model_const::calculate(int ttl)
{
    struct value out = {};
    out.valid = true;
    out.type = (bus_width <= 1) ? VALUE_TYPE_BOOL : VALUE_TYPE_INT;
    out.intval = const_value;
    out.boolval = (const_value != 0);

    if (!ports.empty()) {
        ports[0]->update_value_final(&out, ttl);
        update_value_net(&out, 0, ttl);
    }
}

void model_const::drawAt(class basecanvas *canvas, float X, float Y, int type)
{
    if (bus_width <= 1) {
        if (const_value)
            canvas->draw_image_rotated(selected ? "assets/one_base.png" : "assets/one.png", X, Y, sizeX, sizeY, Alpha(type), angle);
        else
            canvas->draw_image_rotated(selected ? "assets/zero_base.png" : "assets/zero.png", X, Y, sizeX, sizeY, Alpha(type), angle);
    } else {
        canvas->draw_image_rotated(selected ? "assets/toggle_base.png" : "assets/toggle_bus.png", X, Y, sizeX, sizeY, Alpha(type), angle);
        canvas->draw_text(std::format("0x{:X}", const_value), X + 0.15f, Y + 0.15f, sizeX - 0.3f, sizeY - 0.3f);
    }

    for (auto &port : ports) {
        port->drawAt(canvas, X, Y, COLOR_WIRE_SOLID);
        port->draw_wires(canvas);
    }
}

void model_const::to_json(json &j)
{
    element::to_json(j);
    j["bus_width"] = bus_width;
    j["const_value"] = const_value;
}

void model_const::from_json(json &j)
{
    element::from_json(j);
    set_const(j.value("bus_width", 8), j.value("const_value", uint64_t{0}));
}

std::string model_const::get_verilog_main(void)
{
    if (ports.empty() || !ports[0]->has_net())
        return "";
    return std::format("assign {} = {}'h{:x};\n",
                       ports[0]->get_net_verilog_name(), bus_width, const_value);
}
