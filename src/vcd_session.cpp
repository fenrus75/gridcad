/*
 * gridcad -- a tool to do digital circuit design with a GUI
 *
 * (C) Copyright 2025  -- Arjan van de Ven
 *
 * Licensed under the terms of the
 * GNU General Public LIcense version 3.0
 */

#include "vcd_session.h"
#include "gridcad.h"
#include "scene.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <format>
#include <unordered_map>
#include <unordered_set>

struct value vcd_session::parse_vcd_raw_value(std::string_view raw, int bit_width)
{
    struct value v = {};
    v.type = (bit_width <= 1) ? VALUE_TYPE_BOOL : VALUE_TYPE_INT;

    if (raw.empty()) {
        v.valid = false;
        return v;
    }

    if (raw.front() == 'b' || raw.front() == 'B')
        raw.remove_prefix(1);

    uint64_t acc = 0;
    bool has_x = false;
    bool has_z = false;

    for (char c : raw) {
        if (c == '0') {
            acc = (acc << 1);
        } else if (c == '1') {
            acc = (acc << 1) | 1ULL;
        } else if (c == 'x' || c == 'X') {
            acc = (acc << 1);
            has_x = true;
        } else if (c == 'z' || c == 'Z') {
            acc = (acc << 1);
            has_z = true;
        }
    }

    if (bit_width > 0 && bit_width < 64)
        acc &= (uint64_t{1} << bit_width) - 1;

    v.intval = acc;
    v.boolval = (acc != 0);
    v.valid = !has_x && !has_z;
    v.is_error = has_x;
    return v;
}

bool vcd_session::load(std::string_view vcd_path, std::string &out_error,
                       std::string_view explicit_clock)
{
    loaded_ = false;
    clock_ticks_.clear();
    bindings_.clear();
    clock_signal_.clear();
    scope_prefix_.clear();
    current_tick_ = 0;

    if (!parser_.parse_all(vcd_path, out_error))
        return false;

    if (!explicit_clock.empty()) {
        clock_signal_ = std::string(explicit_clock);
    } else if (auto clk = parser_.detect_clock()) {
        clock_signal_ = clk->signal_name;
    }

    if (!clock_signal_.empty())
        clock_ticks_ = parser_.get_clock_edge_times(clock_signal_, "posedge");

    if (clock_ticks_.empty()) {
        /* Fallback for combinational / clockless VCDs: sample start and end times. */
        clock_ticks_.push_back(parser_.get_start_time());
        if (parser_.get_end_time() > parser_.get_start_time())
            clock_ticks_.push_back(parser_.get_end_time());
    }

    loaded_ = true;
    return true;
}

size_t vcd_session::bind_scene(const class scene *root_scene, std::string_view explicit_scope)
{
    bindings_.clear();
    scope_prefix_.clear();

    if (!loaded_ || !root_scene)
        return 0;

    std::vector<std::string> scene_vcd_names;
    root_scene->collect_vcd_names(scene_vcd_names);
    if (scene_vcd_names.empty())
        return 0;

    /* Index all VCD signals by full_name for O(1) lookup. */
    std::unordered_map<std::string, const verilog::vcd_signal *> full_map;
    std::unordered_set<std::string> candidate_scopes;
    candidate_scopes.insert("");

    for (const auto &sig : parser_.get_signals()) {
        full_map.emplace(sig.full_name, &sig);
        if (!sig.scope.empty())
            candidate_scopes.insert(sig.scope);
    }

    if (!explicit_scope.empty()) {
        scope_prefix_ = std::string(explicit_scope);
    } else {
        /*
         * Score every unique scope in the VCD by how many scene wire vcd_names
         * resolve cleanly under "<scope>.<vcd_name>". Prefer the highest match
         * count (and on ties, the deeper/more specific scope, e.g. top_tb.dut
         * over top_tb).
         */
        size_t best_hits = 0;
        std::string best_scope;

        for (const auto &scope : candidate_scopes) {
            size_t hits = 0;
            for (const auto &vn : scene_vcd_names) {
                std::string full = scope.empty() ? vn : std::format("{}.{}", scope, vn);
                if (full_map.count(full))
                    hits++;
            }
            if (hits > best_hits ||
                (hits == best_hits && hits > 0 && scope.size() > best_scope.size())) {
                best_hits = hits;
                best_scope = scope;
            }
        }
        scope_prefix_ = best_scope;
    }

    std::unordered_map<std::string, std::pair<std::string, bool>> qualifiers;
    root_scene->collect_vcd_qualifiers(qualifiers);

    /* Build final binding list. */
    for (const auto &vn : scene_vcd_names) {
        std::string full = scope_prefix_.empty() ? vn : (scope_prefix_ + "." + vn);
        auto it = full_map.find(full);
        if (it != full_map.end()) {
            const verilog::vcd_signal *valid_sig = nullptr;
            bool valid_active_low = false;
            if (auto qit = qualifiers.find(vn); qit != qualifiers.end()) {
                const std::string &qname = qit->second.first;
                std::string full_q = scope_prefix_.empty() ? qname : (scope_prefix_ + "." + qname);
                if (auto vit = full_map.find(full_q); vit != full_map.end()) {
                    valid_sig = vit->second;
                    valid_active_low = qit->second.second;
                }
            }
            bindings_.push_back(binding{vn, it->second, valid_sig, valid_active_low});
        }
    }

    return bindings_.size();
}

bool vcd_session::evaluate_valid_raw(std::string_view raw, bool active_low)
{
    if (raw.empty())
        return false;
    bool any_one = false;
    for (char c : raw) {
        if (c == 'x' || c == 'X' || c == 'z' || c == 'Z')
            return false;
        if (c == '1')
            any_one = true;
    }
    return active_low ? !any_one : any_one;
}

struct value vcd_session::sample_binding_at_time(const binding &b, uint64_t t) const
{
    const std::string raw = parser_.get_raw_value_at_time(*b.signal, t);
    struct value v = parse_vcd_raw_value(raw, b.signal->bit_width);
    if (b.signal->full_name == clock_signal_ || b.signal->short_name == "clk")
        v.is_clock = true;
    if (b.valid_signal) {
        const std::string vraw = parser_.get_raw_value_at_time(*b.valid_signal, t);
        v.qualifier_valid = evaluate_valid_raw(vraw, b.valid_active_low);
    }
    return v;
}

uint64_t vcd_session::get_current_timestamp(void) const noexcept
{
    if (clock_ticks_.empty() || current_tick_ >= clock_ticks_.size())
        return 0;
    return clock_ticks_[current_tick_];
}

uint64_t vcd_session::get_tick_timestamp(size_t tick_idx) const noexcept
{
    if (clock_ticks_.empty())
        return 0;
    return clock_ticks_[std::min(tick_idx, clock_ticks_.size() - 1)];
}

size_t vcd_session::find_nearest_tick(uint64_t timestamp) const noexcept
{
    if (clock_ticks_.empty())
        return 0;
    auto it = std::lower_bound(clock_ticks_.begin(), clock_ticks_.end(), timestamp);
    if (it == clock_ticks_.begin())
        return 0;
    if (it == clock_ticks_.end())
        return clock_ticks_.size() - 1;
    const auto prev = std::prev(it);
    if (timestamp - *prev <= *it - timestamp)
        return static_cast<size_t>(std::distance(clock_ticks_.begin(), prev));
    return static_cast<size_t>(std::distance(clock_ticks_.begin(), it));
}

bool vcd_session::has_binding(std::string_view wire_vcd_name) const noexcept
{
    if (!loaded_ || wire_vcd_name.empty())
        return false;
    for (const auto &b : bindings_) {
        if (b.wire_vcd_name == wire_vcd_name && b.signal != nullptr)
            return true;
    }
    return false;
}

namespace {

bool same_wave_value(const struct value &a, const struct value &b) noexcept
{
    return a.valid == b.valid &&
           a.is_error == b.is_error &&
           a.boolval == b.boolval &&
           a.intval == b.intval &&
           a.qualifier_valid == b.qualifier_valid;
}

vcd_session::wave_data build_wave_from_maps(
    const verilog::vcd_signal &sig,
    const std::map<uint64_t, std::string> *sig_trans,
    const std::map<uint64_t, std::string> *val_trans,
    bool valid_active_low,
    bool is_clock,
    uint64_t t_min,
    uint64_t t_max)
{
    vcd_session::wave_data out;
    out.signal_name = sig.short_name.empty() ? sig.full_name : sig.short_name;
    out.bit_width = std::max(1, sig.bit_width);
    out.t_min = t_min;
    out.t_max = (t_max > t_min) ? t_max : (t_min + 1);
    out.has_qualifier = (val_trans != nullptr);
    out.is_clock = is_clock;

    if (!sig_trans || sig_trans->empty())
        return out;

    auto sig_it = sig_trans->begin();
    const auto sig_end = sig_trans->end();
    const std::map<uint64_t, std::string> empty_map;
    auto val_it = val_trans ? val_trans->begin() : empty_map.begin();
    const auto val_end = val_trans ? val_trans->end() : empty_map.end();

    std::string cur_raw;
    std::string cur_vraw;
    bool have_sig = false;

    while (sig_it != sig_end || val_it != val_end) {
        uint64_t t = 0;
        if (sig_it != sig_end && val_it != val_end)
            t = std::min(sig_it->first, val_it->first);
        else if (sig_it != sig_end)
            t = sig_it->first;
        else
            t = val_it->first;

        if (sig_it != sig_end && sig_it->first == t) {
            cur_raw = sig_it->second;
            have_sig = true;
            ++sig_it;
        }
        if (val_it != val_end && val_it->first == t) {
            cur_vraw = val_it->second;
            ++val_it;
        }
        if (!have_sig)
            continue;

        struct value v = vcd_session::parse_vcd_raw_value(cur_raw, out.bit_width);
        v.is_clock = is_clock;
        if (out.has_qualifier)
            v.qualifier_valid = vcd_session::evaluate_valid_raw(cur_vraw, valid_active_low);

        if (!out.segments.empty()) {
            if (same_wave_value(out.segments.back().val, v)) {
                out.segments.back().t_end = out.t_max;
                continue;
            }
            out.segments.back().t_end = t;
            if (out.segments.back().t_end <= out.segments.back().t_start)
                out.segments.pop_back();
        }
        out.segments.push_back(vcd_session::wave_segment{t, out.t_max, v, out.has_qualifier});
    }

    if (!out.segments.empty())
        out.segments.back().t_end = out.t_max;
    return out;
}

} // namespace

vcd_session::wave_data vcd_session::extract_wave(std::string_view wire_vcd_name) const
{
    if (!loaded_ || wire_vcd_name.empty())
        return {};
    const binding *found = nullptr;
    for (const auto &b : bindings_) {
        if (b.wire_vcd_name == wire_vcd_name && b.signal != nullptr) {
            found = &b;
            break;
        }
    }
    if (!found)
        return {};

    uint64_t t_min = parser_.get_start_time();
    uint64_t t_max = parser_.get_end_time();
    if (!clock_ticks_.empty())
        t_max = std::max(t_max, clock_ticks_.back());

    const auto *sig_trans = parser_.get_signal_transitions(*found->signal);
    const auto *val_trans = found->valid_signal ? parser_.get_signal_transitions(*found->valid_signal) : nullptr;
    const bool is_clk = (found->signal->full_name == clock_signal_ || found->signal->short_name == "clk");
    return build_wave_from_maps(*found->signal, sig_trans, val_trans, found->valid_active_low, is_clk, t_min, t_max);
}

vcd_session::wave_data vcd_session::extract_clock_wave(void) const
{
    if (!loaded_ || clock_signal_.empty())
        return {};
    const auto *clk_sig = parser_.find_signal(clock_signal_, scope_prefix_);
    if (!clk_sig)
        return {};

    uint64_t t_min = parser_.get_start_time();
    uint64_t t_max = parser_.get_end_time();
    if (!clock_ticks_.empty())
        t_max = std::max(t_max, clock_ticks_.back());

    const auto *clk_trans = parser_.get_signal_transitions(*clk_sig);
    return build_wave_from_maps(*clk_sig, clk_trans, nullptr, false, true, t_min, t_max);
}

size_t vcd_session::apply_tick(size_t tick_idx, class scene *root_scene)
{
    if (!loaded_ || !root_scene || clock_ticks_.empty())
        return 0;

    current_tick_ = std::min(tick_idx, clock_ticks_.size() - 1);
    unlocked_ = false;
    const uint64_t t = clock_ticks_[current_tick_];

    std::unordered_map<std::string, struct value> batch;
    batch.reserve(bindings_.size());
    for (const auto &b : bindings_)
        batch.emplace(b.wire_vcd_name, sample_binding_at_time(b, t));

    size_t updated = root_scene->broadcast_vcd_values(batch);

    /* Let unlocked glue wires and zoomed-in sub-blocks settle from the VCD values. */
    run_queued_calculations();
    return updated;
}

size_t vcd_session::push_tick_events(size_t tick_idx)
{
    if (!loaded_ || clock_ticks_.empty())
        return 0;

    current_tick_ = std::min(tick_idx, clock_ticks_.size() - 1);
    unlocked_ = false;
    const uint64_t t = clock_ticks_[current_tick_];

    auto payload = std::make_unique<vcd_wire_update>();
    payload->values.reserve(bindings_.size());
    for (const auto &b : bindings_)
        payload->values.emplace(b.wire_vcd_name, sample_binding_at_time(b, t));

    const size_t count = payload->values.size();
    if (count == 0)
        return 0;

    SDL_Event ev = {};
    ev.type = EVENT_VCD_WIRE_UPDATE;
    ev.user.code = static_cast<Sint32>(current_tick_);
    ev.user.data1 = payload.release();
    if (SDL_PushEvent(&ev))
        return count;

    /* Reclaim payload on queue failure so we never leak. */
    std::unique_ptr<vcd_wire_update> reclaim(static_cast<vcd_wire_update *>(ev.user.data1));
    return 0;
}

size_t vcd_session::step_forward(class scene *root_scene)
{
    if (clock_ticks_.empty())
        return 0;
    size_t next = (current_tick_ + 1 < clock_ticks_.size()) ? (current_tick_ + 1) : 0;
    return apply_tick(next, root_scene);
}

size_t vcd_session::step_backward(class scene *root_scene)
{
    if (clock_ticks_.empty())
        return 0;
    size_t prev = (current_tick_ > 0) ? (current_tick_ - 1) : (clock_ticks_.size() - 1);
    return apply_tick(prev, root_scene);
}

static vcd_session *g_active_vcd_session = nullptr;

vcd_session *get_active_vcd_session(void)
{
    return g_active_vcd_session;
}

void set_active_vcd_session(vcd_session *sess)
{
    g_active_vcd_session = sess;
}
