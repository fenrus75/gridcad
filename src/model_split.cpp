/*
 * gridcad -- a tool to do digital circuit design with a GUI
 *
 * (C) Copyright 2025  -- Arjan van de Ven
 *
 * Licensed under the terms of the
 * GNU General Public LIcense version 3.0
 */

#include "gridcad.h"
#include "model_split.h"
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

model_split::model_split(float _X, float _Y, int _in_width, int outputs,
                         const std::vector<int> &offsets,
                         const std::vector<int> &out_widths)
    : element(2, std::max(2, std::clamp(outputs, 1, 64)), "split")
{
    in_width = std::clamp(_in_width, 1, 64);
    const int n_out = std::clamp(outputs, 1, 64);

    bit_offsets.resize(static_cast<size_t>(n_out));
    output_widths.resize(static_cast<size_t>(n_out));
    int running_off = 0;
    for (int i = 0; i < n_out; ++i) {
        const int ow = (static_cast<size_t>(i) < out_widths.size())
                           ? std::clamp(out_widths[static_cast<size_t>(i)], 1, 64)
                           : 1;
        const int off = (static_cast<size_t>(i) < offsets.size())
                            ? std::clamp(offsets[static_cast<size_t>(i)], 0, 63)
                            : running_off;
        output_widths[static_cast<size_t>(i)] = ow;
        bit_offsets[static_cast<size_t>(i)] = off;
        running_off = off + ow;
    }

    sizeX = 2;
    sizeY = std::max(2, n_out);
    X = floorf(_X);
    Y = floorf(_Y);

    add_port(-1, sizeY / 2, "in", PORT_IN, in_width);
    for (int i = 0; i < n_out; ++i) {
        add_port(sizeX, i, std::format("out{}", i), PORT_OUT,
                 output_widths[static_cast<size_t>(i)]);
    }
}

void model_split::drawAt(class basecanvas *canvas, float _X, float _Y, int type)
{
    const bool large = (output_widths.size() > 4);
    const char *img = selected
                          ? (large ? "assets/8to1_selected.png" : "assets/4to1_selected.png")
                          : (large ? "assets/8to1.png" : "assets/4to1.png");
    canvas->draw_image(img, _X, _Y, sizeX, sizeY, Alpha(type));

    for (auto &port : ports) {
        port->draw_wires(canvas);
        port->drawAt(canvas, _X, _Y, type);
    }
    hover_ports(canvas);
}

void model_split::calculate(int ttl)
{
    if (ttl <= 0 || ports.size() < output_widths.size() + 1)
        return;

    const uint64_t in_val = read_u64(ports[0]->value, in_width);

    for (size_t i = 0; i < output_widths.size(); ++i) {
        const int ow = output_widths[i];
        const int off = bit_offsets[i];
        const uint64_t seg = (off < 64) ? ((in_val >> off) & width_mask(ow)) : 0ULL;

        struct value out = {};
        out.valid = true;
        out.type = (ow <= 1) ? VALUE_TYPE_BOOL : VALUE_TYPE_INT;
        out.intval = seg;
        out.boolval = (seg != 0);

        update_value_net(&out, static_cast<int>(1 + i), ttl - 1);
    }
}

void model_split::to_json(json &j)
{
    element::to_json(j);
    j["in_width"] = in_width;
    j["bit_offsets"] = bit_offsets;
    j["output_widths"] = output_widths;
}

void model_split::from_json(json &j)
{
    element::from_json(j);
    in_width = std::clamp(j.value("in_width", 8), 1, 64);
    if (j.contains("bit_offsets") && j["bit_offsets"].is_array()) {
        bit_offsets.clear();
        for (const auto &v : j["bit_offsets"]) {
            if (v.is_number_integer())
                bit_offsets.push_back(std::clamp(v.get<int>(), 0, 63));
        }
    }
    if (j.contains("output_widths") && j["output_widths"].is_array()) {
        output_widths.clear();
        for (const auto &v : j["output_widths"]) {
            if (v.is_number_integer())
                output_widths.push_back(std::clamp(v.get<int>(), 1, 64));
        }
    }
}
