/*
 * gridcad -- a tool to do digital circuit design with a GUI
 *
 * (C) Copyright 2025  -- Arjan van de Ven
 *
 * Licensed under the terms of the
 * GNU General Public LIcense version 3.0
 */

#include "gridcad.h"
#include "model_mux.h"
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

model_mux::model_mux(float _X, float _Y, int width)
    : element(3, 3, "mux")
{
    sizeX = 3;
    sizeY = 3;
    X = floorf(_X);
    Y = floorf(_Y);

    bus_width = std::clamp(width, 1, 64);

    add_port(-1, 0, "a", PORT_IN, bus_width);
    add_port(-1, 2, "b", PORT_IN, bus_width);
    add_port(1, -1, "s", PORT_IN, 1);
    add_port(sizeX, 1, "y", PORT_OUT, bus_width);
}

void model_mux::set_bus_width(int width)
{
    bus_width = std::clamp(width, 1, 64);
    ports[0]->set_width(bus_width);
    ports[1]->set_width(bus_width);
    ports[3]->set_width(bus_width);
}

void model_mux::calculate(int ttl)
{
    bool sel = ports[2]->value.boolval;
    uint64_t chosen = sel ? read_u64(ports[1]->value, bus_width)
                          : read_u64(ports[0]->value, bus_width);

    struct value out = {};
    out.valid = true;
    out.type = (bus_width <= 1) ? VALUE_TYPE_BOOL : VALUE_TYPE_INT;
    out.intval = chosen;
    out.boolval = (chosen != 0);

    update_value_net(&out, 3, ttl - 1);
}

void model_mux::drawAt(class basecanvas *canvas, float X, float Y, int type)
{
    if (selected)
        canvas->draw_image_rotated("assets/selectionhalo.png", X - 0.1f, Y - 0.1f, sizeX + 0.2f, sizeY + 0.2f, Alpha(type), angle);
    canvas->draw_image_rotated("assets/mux_body.png", X, Y, sizeX, sizeY, Alpha(type), angle);

    /* Draw select arrow from top select port 's' (col 1) into the sloped top edge. */
    const float frac = 1.5f / static_cast<float>(sizeX);
    const float edge_y = Y + 0.25f * static_cast<float>(sizeY) * frac;
    const float arrow_h = std::max(0.55f, (edge_y + 0.30f) - (Y - 0.05f));
    canvas->draw_image_rotated("assets/mux_arrow.png", X + 1.20f, Y - 0.05f, 0.60f, arrow_h, Alpha(type), 0);

    canvas->draw_text(std::format("{}", bus_width),
                      X + 0.3f, Y + sizeY * 0.5f - 0.45f, sizeX - 0.6f, 0.9f);

    for (auto &port : ports) {
        port->drawAt(canvas, X, Y, COLOR_WIRE_SOLID);
        port->draw_wires(canvas);
    }
}

void model_mux::to_json(json &j)
{
    element::to_json(j);
    j["bus_width"] = bus_width;
}

void model_mux::from_json(json &j)
{
    element::from_json(j);
    set_bus_width(j.value("bus_width", 8));
}

std::string model_mux::get_verilog_main(void)
{
    if (!ports[3]->has_net())
        return "";
    return "assign " + ports[3]->get_net_verilog_name() + " = "
         + ports[2]->get_net_verilog_name("1'b0") + " ? "
         + ports[1]->get_net_verilog_name("0") + " : "
         + ports[0]->get_net_verilog_name("0") + ";\n";
}
