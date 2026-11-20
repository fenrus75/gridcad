/*
 * gridcad -- a tool to do digital circuit design with a GUI
 *
 * (C) Copyright 2025  -- Arjan van de Ven
 *
 * Licensed under the terms of the
 * GNU General Public LIcense version 3.0
 */

#include "gridcad.h"
#include "model_pmux.h"
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

model_pmux::model_pmux(float _X, float _Y, int width, int arms)
    : element(3, 3, "pmux")
{
    X = floorf(_X);
    Y = floorf(_Y);

    bus_width = std::clamp(width, 1, 64);
    num_arms = std::clamp(arms, 2, 64);

    rebuild_ports();
}

void model_pmux::rebuild_ports(void)
{
    ports.clear();

    const int top_sels = (num_arms <= 8) ? num_arms : ((num_arms + 1) / 2);
    const int bot_sels = num_arms - top_sels;

    sizeX = std::max(3, top_sels);
    sizeY = std::max(3, num_arms + 1);

    /* Port 0: default input 'a' */
    add_port(-1, 0, "a", PORT_IN, bus_width);

    /* Ports 1..num_arms: arm inputs 'b0'..'b{num_arms-1}' */
    for (int i = 0; i < num_arms; ++i)
        add_port(-1, 1 + i, std::format("b{}", i), PORT_IN, bus_width);

    /* Ports 1+num_arms..2*num_arms: select inputs 's0'..'s{num_arms-1}' */
    for (int i = 0; i < top_sels; ++i)
        add_port(i, -1, std::format("s{}", i), PORT_IN, 1);
    for (int i = 0; i < bot_sels; ++i)
        add_port(i, sizeY, std::format("s{}", top_sels + i), PORT_IN, 1);

    /* Port 1+2*num_arms: output 'y' */
    add_port(sizeX, sizeY / 2, "y", PORT_OUT, bus_width);

    reseat();
}

void model_pmux::calculate(int ttl)
{
    const size_t y_idx = static_cast<size_t>(1 + 2 * num_arms);
    if (ports.size() <= y_idx)
        return;

    uint64_t chosen = read_u64(ports[0]->value, bus_width);
    for (int i = 0; i < num_arms; ++i) {
        const size_t s_idx = static_cast<size_t>(1 + num_arms + i);
        if (ports[s_idx]->value.boolval) {
            chosen = read_u64(ports[static_cast<size_t>(1 + i)]->value, bus_width);
            break;
        }
    }

    struct value out = {};
    out.valid = true;
    out.type = (bus_width <= 1) ? VALUE_TYPE_BOOL : VALUE_TYPE_INT;
    out.intval = chosen;
    out.boolval = (chosen != 0);

    update_value_net(&out, static_cast<int>(y_idx), ttl - 1);
}

void model_pmux::drawAt(class basecanvas *canvas, float X, float Y, int type)
{
    canvas->draw_image_rotated(selected ? "library/mux/MUX8.json.selected.png" : "library/mux/MUX8.json.png",
                               X, Y, sizeX, sizeY, Alpha(type), angle);
    canvas->draw_text(std::format("{}:1 ({}b)", num_arms, bus_width),
                      X + 0.2f, Y + sizeY * 0.5f - 0.6f, sizeX - 0.4f, 1.2f);

    for (auto &port : ports) {
        port->drawAt(canvas, X, Y, COLOR_WIRE_SOLID);
        port->draw_wires(canvas);
    }
}

void model_pmux::to_json(json &j)
{
    element::to_json(j);
    j["bus_width"] = bus_width;
    j["num_arms"] = num_arms;
}

void model_pmux::from_json(json &j)
{
    bus_width = std::clamp(j.value("bus_width", 8), 1, 64);
    num_arms = std::clamp(j.value("num_arms", 2), 2, 64);
    rebuild_ports();
    element::from_json(j);
}

std::string model_pmux::get_verilog_main(void)
{
    const size_t y_idx = static_cast<size_t>(1 + 2 * num_arms);
    if (ports.size() <= y_idx || !ports[y_idx]->has_net())
        return "";

    std::string expr = ports[0]->get_net_verilog_name("0");
    for (int i = num_arms - 1; i >= 0; --i) {
        const size_t s_idx = static_cast<size_t>(1 + num_arms + i);
        const size_t b_idx = static_cast<size_t>(1 + i);
        expr = "(" + ports[s_idx]->get_net_verilog_name("1'b0") + " ? "
             + ports[b_idx]->get_net_verilog_name("0") + " : " + expr + ")";
    }
    return "assign " + ports[y_idx]->get_net_verilog_name() + " = " + expr + ";\n";
}
