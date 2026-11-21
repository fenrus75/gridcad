/*
 * gridcad -- a tool to do digital circuit design with a GUI
 *
 * (C) Copyright 2025  -- Arjan van de Ven
 *
 * Licensed under the terms of the
 * GNU General Public LIcense version 3.0
 */

#include "gridcad.h"
#include "model_reduce.h"
#include "port.h"

#include <algorithm>
#include <bit>
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

static const char *op_symbol(reduce_op op)
{
    switch (op) {
    case reduce_op::OR:   return "|";
    case reduce_op::AND:  return "&";
    case reduce_op::XOR:  return "^";
    case reduce_op::XNOR: return "~^";
    }
    return "|";
}

model_reduce::model_reduce(float _X, float _Y, int width, reduce_op _op)
    : element(3, 3, "reduce"), op(_op)
{
    sizeX = 3;
    sizeY = 3;
    X = floorf(_X);
    Y = floorf(_Y);

    bus_width = std::clamp(width, 1, 64);

    add_port(-1, 1, "a", PORT_IN, bus_width);
    add_port(sizeX, 1, "y", PORT_OUT, 1);
}

void model_reduce::set_bus_width(int width)
{
    bus_width = std::clamp(width, 1, 64);
    ports[0]->set_width(bus_width);
}

void model_reduce::calculate(int ttl)
{
    const uint64_t val = read_u64(ports[0]->value, bus_width);
    const uint64_t mask = width_mask(bus_width);

    bool res = false;
    switch (op) {
    case reduce_op::OR:
        res = (val != 0);
        break;
    case reduce_op::AND:
        res = (val == mask);
        break;
    case reduce_op::XOR:
        res = (std::popcount(val) & 1) != 0;
        break;
    case reduce_op::XNOR:
        res = (std::popcount(val) & 1) == 0;
        break;
    }

    struct value out = {};
    out.valid = true;
    out.type = VALUE_TYPE_BOOL;
    out.boolval = res;
    out.intval = res ? 1 : 0;

    update_value_net(&out, 1, ttl - 1);
}

void model_reduce::drawAt(class basecanvas *canvas, float X, float Y, int type)
{
    canvas->draw_image_rotated(selected ? "assets/buttonbox_hover.png" : "assets/buttonbox.png",
                               X, Y, sizeX, sizeY, Alpha(type), angle);
    canvas->draw_text(std::format("{}{}", op_symbol(op), bus_width),
                      X + 0.15f, Y + 0.85f, sizeX - 0.3f, 1.3f);

    for (auto &port : ports) {
        port->drawAt(canvas, X, Y, COLOR_WIRE_SOLID);
        port->draw_wires(canvas);
    }
}

void model_reduce::to_json(json &j)
{
    element::to_json(j);
    j["bus_width"] = bus_width;
    j["reduce_op"] = static_cast<int>(op);
}

void model_reduce::from_json(json &j)
{
    element::from_json(j);
    set_bus_width(j.value("bus_width", 8));
    const int raw_op = std::clamp(j.value("reduce_op", 0), 0, 3);
    op = static_cast<reduce_op>(raw_op);
}

std::string model_reduce::get_verilog_main(void)
{
    if (!ports[1]->has_net())
        return "";
    return "assign " + ports[1]->get_net_verilog_name() + " = "
         + std::string(op_symbol(op)) + "("
         + ports[0]->get_net_verilog_name("0") + ");\n";
}
