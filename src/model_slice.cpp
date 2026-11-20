/*
 * gridcad -- a tool to do digital circuit design with a GUI
 *
 * (C) Copyright 2025  -- Arjan van de Ven
 *
 * Licensed under the terms of the
 * GNU General Public LIcense version 3.0
 */

#include "gridcad.h"
#include "model_slice.h"
#include "port.h"

#include <algorithm>
#include <format>

namespace {

uint64_t width_mask(int width)
{
    if (width <= 0)
        return 0;
    if (width >= 64)
        return ~uint64_t{0};
    return (uint64_t{1} << width) - 1;
}

uint64_t read_u64(const struct value &v, int width)
{
    uint64_t raw = (v.type == VALUE_TYPE_BOOL) ? (v.boolval ? 1ULL : 0ULL) : v.intval;
    return raw & width_mask(width);
}

} // namespace

model_slice::model_slice(float _X, float _Y, int _in_width, int _out_width, int _offset)
    : element(2, 1, "slice")
{
    sizeX = 2;
    sizeY = 1;
    X = floorf(_X);
    Y = floorf(_Y);

    in_width = std::clamp(_in_width, 1, 64);
    out_width = std::clamp(_out_width, 1, 64);
    offset = std::clamp(_offset, 0, 63);

    add_port(-1, 0, "in", PORT_IN, in_width);
    add_port(sizeX, 0, "out", PORT_OUT, out_width);
}

void model_slice::drawAt(class basecanvas *canvas, float _X, float _Y, int type)
{
    canvas->draw_image("assets/buttonbox.png", _X, _Y, sizeX, sizeY, Alpha(type));
    const std::string lbl = (out_width <= 1)
                                ? std::format("[{}]", offset)
                                : std::format("[{}:{}]", offset + out_width - 1, offset);
    canvas->draw_text(lbl, _X + 0.1f, _Y + 0.1f, sizeX - 0.2f, sizeY - 0.2f);

    for (auto &port : ports) {
        port->draw_wires(canvas);
        port->drawAt(canvas, _X, _Y, type);
    }
    hover_ports(canvas);
}

void model_slice::calculate(int ttl)
{
    if (ttl <= 0 || ports.size() < 2)
        return;

    const uint64_t in_val = read_u64(ports[0]->value, in_width);
    const uint64_t sliced = (offset < 64) ? ((in_val >> offset) & width_mask(out_width)) : 0ULL;

    struct value out = {};
    out.valid = true;
    out.type = (out_width <= 1) ? VALUE_TYPE_BOOL : VALUE_TYPE_INT;
    out.intval = sliced;
    out.boolval = (sliced != 0);

    update_value_net(&out, 1, ttl - 1);
}

void model_slice::to_json(json &j)
{
    element::to_json(j);
    j["in_width"] = in_width;
    j["out_width"] = out_width;
    j["offset"] = offset;
}

void model_slice::from_json(json &j)
{
    element::from_json(j);
    in_width = std::clamp(j.value("in_width", 8), 1, 64);
    out_width = std::clamp(j.value("out_width", 1), 1, 64);
    offset = std::clamp(j.value("offset", 0), 0, 63);
    if (ports.size() >= 2) {
        ports[0]->set_width(in_width);
        ports[1]->set_width(out_width);
    }
}
