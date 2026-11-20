/*
 * gridcad -- a tool to do digital circuit design with a GUI
 *
 * (C) Copyright 2025  -- Arjan van de Ven
 *
 * Licensed under the terms of the
 * GNU General Public LIcense version 3.0
 */

#include "gridcad.h"
#include "model_reg.h"
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

model_reg::model_reg(float _X, float _Y, int width, uint64_t reset_val)
    : element(3, 4, "reg")
{
    sizeX = 3;
    sizeY = 4;
    X = floorf(_X);
    Y = floorf(_Y);

    bus_width = std::clamp(width, 1, 64);
    reset_value = reset_val & width_mask(bus_width);

    stored_value = {};
    stored_value.valid = true;
    stored_value.type = (bus_width <= 1) ? VALUE_TYPE_BOOL : VALUE_TYPE_INT;
    stored_value.intval = reset_value;
    stored_value.boolval = (reset_value != 0);

    add_port(-1, 1, "d", PORT_IN, bus_width);
    add_port(1, -1, "en", PORT_IN, 1);
    add_port(-1, 3, "clk", PORT_IN, 1);
    add_port(1, sizeY, "rst", PORT_IN, 1);
    add_port(sizeX, 1, "q", PORT_OUT, bus_width);
    ports[4]->update_value_final(&stored_value, DEFAULT_TTL);
}

void model_reg::set_params(int width, uint64_t reset_val)
{
    bus_width = std::clamp(width, 1, 64);
    reset_value = reset_val & width_mask(bus_width);
    ports[0]->set_width(bus_width);
    ports[4]->set_width(bus_width);

    stored_value.valid = true;
    stored_value.type = (bus_width <= 1) ? VALUE_TYPE_BOOL : VALUE_TYPE_INT;
    stored_value.intval = reset_value;
    stored_value.boolval = (reset_value != 0);
}

void model_reg::calculate(int ttl)
{
    // Async active-high reset
    if (ports[3]->value.boolval) {
        struct value rst_v = {};
        rst_v.valid = true;
        rst_v.type = (bus_width <= 1) ? VALUE_TYPE_BOOL : VALUE_TYPE_INT;
        rst_v.intval = reset_value;
        rst_v.boolval = (reset_value != 0);

        if (!prev_rst || !stored_value.valid || stored_value.intval != rst_v.intval) {
            stored_value = rst_v;
            queue_calculate(this);
        } else {
            stored_value = rst_v;
        }
        prev_rst = true;
        previous_clock = ports[2]->value;
        return;
    }
    prev_rst = false;

    struct value newclock = ports[2]->value;
    if (newclock.boolval == previous_clock.boolval)
        return;

    if (newclock.boolval) { // rising edge
        bool enabled = !ports[1]->has_wires() || ports[1]->value.boolval;
        if (enabled) {
            uint64_t next_raw = read_u64(ports[0]->value, bus_width);
            struct value next_v = {};
            next_v.valid = true;
            next_v.type = (bus_width <= 1) ? VALUE_TYPE_BOOL : VALUE_TYPE_INT;
            next_v.intval = next_raw;
            next_v.boolval = (next_raw != 0);

            if (stored_value.intval != next_v.intval || !stored_value.valid) {
                stored_value = next_v;
                queue_calculate(this);
            }
        }
    }
    previous_clock = newclock;
}

void model_reg::queued_calculate(int ttl)
{
    update_value_net(&stored_value, 4, ttl - 1);
}

void model_reg::drawAt(class basecanvas *canvas, float X, float Y, int type)
{
    canvas->draw_image_rotated(selected ? "assets/deflipflop_selected.png" : "assets/deflipflop.png",
                               X, Y, sizeX, sizeY, Alpha(type), angle);
    canvas->draw_text(std::format("0x{:X}", stored_value.intval), X + 0.1f, Y + sizeY - 1.2f, sizeX - 0.2f, 1.0f);

    for (auto &port : ports) {
        port->drawAt(canvas, X, Y, COLOR_WIRE_SOLID);
        port->draw_wires(canvas);
    }
}

void model_reg::to_json(json &j)
{
    element::to_json(j);
    j["bus_width"] = bus_width;
    j["reset_value"] = reset_value;
    j["stored_value"] = stored_value;
    j["previous_clock"] = previous_clock;
}

void model_reg::from_json(json &j)
{
    element::from_json(j);
    set_params(j.value("bus_width", 8), j.value("reset_value", uint64_t{0}));
    stored_value = j.value("stored_value", stored_value);
    previous_clock = j.value("previous_clock", previous_clock);
}

std::string model_reg::get_verilog_main(void)
{
    return "";
}
