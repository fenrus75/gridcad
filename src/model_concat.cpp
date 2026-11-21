/*
 * gridcad -- a tool to do digital circuit design with a GUI
 *
 * (C) Copyright 2025  -- Arjan van de Ven
 *
 * Licensed under the terms of the
 * GNU General Public LIcense version 3.0
 */

#include "gridcad.h"
#include "model_concat.h"
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

model_concat::model_concat(float _X, float _Y, const std::vector<int> &in_widths, int _out_width)
    : element(2, std::max(2, static_cast<int>(in_widths.size())), "concat")
{
    input_widths.clear();
    for (int w : in_widths)
        input_widths.push_back(std::clamp(w, 1, 64));
    if (input_widths.empty())
        input_widths.push_back(1);

    out_width = std::clamp(_out_width, 1, 64);

    sizeX = 2;
    sizeY = std::max(2, static_cast<int>(input_widths.size()));
    X = floorf(_X);
    Y = floorf(_Y);

    for (size_t i = 0; i < input_widths.size(); ++i)
        add_port(-1, static_cast<int>(i), std::format("in{}", i), PORT_IN, input_widths[i]);
    add_port(sizeX, sizeY / 2, "out", PORT_OUT, out_width);
}

void model_concat::drawAt(class basecanvas *canvas, float _X, float _Y, int type)
{
    const bool large = (input_widths.size() > 4);
    const char *img = selected
                          ? (large ? "assets/1to8_selected.png" : "assets/1to4_selected.png")
                          : (large ? "assets/1to8.png" : "assets/1to4.png");
    canvas->draw_image(img, _X, _Y, sizeX, sizeY, Alpha(type));

    for (auto &port : ports) {
        port->draw_wires(canvas);
        port->drawAt(canvas, _X, _Y, type);
    }
    hover_ports(canvas);
}

void model_concat::calculate(int ttl)
{
    if (ttl <= 0 || ports.size() < input_widths.size() + 1)
        return;

    uint64_t packed = 0;
    int shift = 0;
    for (size_t i = 0; i < input_widths.size(); ++i) {
        const int w = input_widths[i];
        if (shift < 64) {
            const uint64_t seg = read_u64(ports[i]->value, w);
            packed |= (seg << shift);
        }
        shift += w;
    }
    packed &= width_mask(out_width);

    struct value out = {};
    out.valid = true;
    out.type = (out_width <= 1) ? VALUE_TYPE_BOOL : VALUE_TYPE_INT;
    out.intval = packed;
    out.boolval = (packed != 0);

    update_value_net(&out, static_cast<int>(input_widths.size()), ttl - 1);
}

void model_concat::to_json(json &j)
{
    element::to_json(j);
    j["input_widths"] = input_widths;
    j["out_width"] = out_width;
}

void model_concat::from_json(json &j)
{
    if (j.contains("input_widths") && j["input_widths"].is_array()) {
        input_widths.clear();
        for (const auto &v : j["input_widths"]) {
            if (v.is_number_integer())
                input_widths.push_back(std::clamp(v.get<int>(), 1, 64));
        }
    }
    if (input_widths.empty())
        input_widths.push_back(1);
    out_width = std::clamp(j.value("out_width", 2), 1, 64);
    element::from_json(j);
}
