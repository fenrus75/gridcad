/*
 * gridcad -- a tool to do digital circuit design with a GUI
 *
 * (C) Copyright 2025  -- Arjan van de Ven
 *
 * Licensed under the terms of the
 * GNU General Public LIcense version 3.0
 */

#pragma once

#include "vcd_parser.h"
#include "value.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

class scene;

/* Batched payload for EVENT_VCD_WIRE_UPDATE SDL user events (one event per tick). */
struct vcd_wire_update {
    std::unordered_map<std::string, struct value> values;
};

/*
 * Manages a loaded VCD simulation file, clock-tick timeline, and signal-to-wire
 * bindings for a gridcad scene hierarchy.
 */
class vcd_session
{
public:
    vcd_session() = default;

    /* Load and parse an untrusted VCD file and extract clock-edge tick timestamps. */
    bool load(std::string_view vcd_path, std::string &out_error,
              std::string_view explicit_clock = "");

    /*
     * Bind scene wires (by their vcd_name) to VCD signals.
     * If explicit_scope is empty, automatically finds the best matching VCD scope prefix
     * (e.g. "top_tb.dut" or "top_tb.dut.cpu_8080.sp").
     */
    size_t bind_scene(const class scene *root_scene, std::string_view explicit_scope = "");

    /* Broadcast all bound signal values at tick_idx directly into root_scene and settle glue logic. */
    size_t apply_tick(size_t tick_idx, class scene *root_scene);

    /* Push EVENT_VCD_WIRE_UPDATE SDL events for all bound signals at tick_idx. */
    size_t push_tick_events(size_t tick_idx);

    size_t step_forward(class scene *root_scene);
    size_t step_backward(class scene *root_scene);

    bool is_loaded(void) const noexcept { return loaded_; }
    bool is_unlocked(void) const noexcept { return unlocked_; }
    void set_unlocked(bool u) noexcept { unlocked_ = u; }
    size_t get_current_tick(void) const noexcept { return current_tick_; }
    size_t get_total_ticks(void) const noexcept { return clock_ticks_.size(); }
    uint64_t get_current_timestamp(void) const noexcept;
    const std::string &get_timescale(void) const noexcept { return parser_.get_timescale(); }
    const std::string &get_clock_signal(void) const noexcept { return clock_signal_; }
    const std::string &get_scope_prefix(void) const noexcept { return scope_prefix_; }
    size_t get_bound_signal_count(void) const noexcept { return bindings_.size(); }

    /* Convert a raw VCD binary string (e.g. "0", "1", "00110110") into a gridcad value. */
    static struct value parse_vcd_raw_value(std::string_view raw, int bit_width);
    /* Evaluate a 1-bit VCD qualifier string (X/Z or empty -> false). */
    static bool evaluate_valid_raw(std::string_view raw, bool active_low);

private:
    struct binding {
        std::string wire_vcd_name;
        const verilog::vcd_signal *signal = nullptr;
        const verilog::vcd_signal *valid_signal = nullptr;
        bool valid_active_low = false;
    };

    struct value sample_binding_at_time(const binding &b, uint64_t t) const;

    verilog::vcd_parser parser_;
    std::string clock_signal_;
    std::string scope_prefix_;
    std::vector<uint64_t> clock_ticks_;
    std::vector<binding> bindings_;
    size_t current_tick_ = 0;
    bool loaded_ = false;
    bool unlocked_ = false;
};

/* Global active VCD session shared across all open schematic windows. */
vcd_session *get_active_vcd_session(void);
void set_active_vcd_session(vcd_session *sess);
