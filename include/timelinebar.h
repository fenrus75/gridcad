/*
 * gridcad -- a tool to do digital circuit design with a GUI
 *
 * (C) Copyright 2025  -- Arjan van de Ven
 *
 * Licensed under the terms of the
 * GNU General Public LIcense version 3.0
 */

#pragma once

#include "gridcad.h"
#include "vcd_session.h"

#include <SDL3/SDL.h>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

class label_table;

/*
 * Native SDL3 docked timeline bar & GTKWave-style signal wave viewer drawn at
 * the bottom of a canvas when a VCD session is active.
 *
 * Provides transport buttons, click/drag tick scrubbing, tick/timestamp
 * readout, VCD lock/unlock toggle, dynamic hover waveform preview, and
 * multi-signal pinned waveforms with enum and valid-qualifier resolution.
 */
class timelinebar
{
public:
    static constexpr int BAR_HEIGHT = 52;
    static constexpr int CLK_RULER_HEIGHT = 24;
    static constexpr int WAVE_ROW_HEIGHT = 34;
    static constexpr int SIGNAL_COL_WIDTH = 190;
    static constexpr size_t MAX_PINNED = 6;

    struct wave_signal_ref {
        std::string vcd_name;
        std::string display_name;
        int bit_width = 1;
        std::shared_ptr<const class label_table> labels;
    };

    /*
     * 1D flat-top bifocal fisheye lens mapping normalized time u in [0, 1]
     * to normalized screen X in [0, 1] with anchored endpoints (x(0)=0, x(1)=1).
     * Inside [center - half_flat, center + half_flat], magnification is uniform;
     * outside, a raised-cosine ramp transitions smoothly to compressed context tails.
     */
    struct wave_lens {
        double center = 0.5;
        double half_flat = 0.0;
        double ramp = 0.0;
        double boost = 0.0; /* 0.0 => pure linear mapping */

        double x_from_u(double u) const noexcept;
        double u_from_x(double x) const noexcept;
        bool is_active(void) const noexcept { return boost > 1e-6 && half_flat > 0.0; }
    };

    timelinebar() = default;

    void set_session(class vcd_session *sess);
    bool is_active(void) const;

    /* Current total height of the docked bar (expands when hover/pinned waves are shown). */
    int get_height(void) const;

    void zoom_in(void);
    void zoom_out(void);
    size_t get_lens_ticks(void) const noexcept { return lens_ticks_; }
    wave_lens build_lens(double center_u) const noexcept;

    void set_hovered_signal(const wave_signal_ref &sig);
    void clear_hovered_signal(void);
    void pin_signal(const wave_signal_ref &sig, bool notify = true);
    void unpin_signal(std::string_view vcd_name, bool notify = true);
    void clear_pinned_signals(bool notify = true);
    bool is_pinned(std::string_view vcd_name) const;
    bool has_pinned_signals(void) const noexcept { return !pinned_.empty(); }
    std::vector<std::string> get_pinned_vcd_names(void) const;
    void set_on_pinned_changed(std::function<void()> cb) { on_pinned_changed_ = std::move(cb); }
    bool has_active_wave(void) const;

    void draw(class basecanvas *canvas, const SDL_Rect &bar_rect);

    /* Returns true if the event fell inside the timeline bar and was consumed. */
    bool handle_event(const SDL_Event &event, const SDL_Rect &bar_rect);

    /* Maps a screen X coordinate inside the scrubber track to a clamped tick index. */
    size_t tick_from_x(int screen_x, const SDL_Rect &bar_rect) const;

private:
    struct cached_wave_entry {
        wave_signal_ref ref;
        vcd_session::wave_data data;
        std::vector<std::string> seg_labels;
        bool pinned = false;
    };

    struct layout {
        SDL_Rect controls_rect{};
        SDL_Rect wave_panel{};
        SDL_Rect wave_area{};
        SDL_Rect hover_row{};
        SDL_Rect clk_row{};
        SDL_Rect btn_zoom_out{};
        SDL_Rect btn_zoom_in{};
        SDL_Rect btn_start{};
        SDL_Rect btn_prev{};
        SDL_Rect btn_play{};
        SDL_Rect btn_next{};
        SDL_Rect btn_end{};
        SDL_Rect track{};
        SDL_Rect readout{};
        SDL_Rect btn_lock{};
        std::vector<SDL_Rect> pinned_rows;
        std::vector<SDL_Rect> unpin_btns;
    };

    layout compute_layout(const SDL_Rect &bar_rect) const;
    std::pair<uint64_t, uint64_t> compute_time_window(void) const;
    size_t tick_from_wave_x(int screen_x, const SDL_Rect &wave_area, const wave_lens &lens) const;
    double current_playhead_u(void) const;
    bool has_unpinned_hover(void) const;
    void rebuild_entry(cached_wave_entry &entry, const wave_signal_ref &sig, bool pinned);
    void ensure_clock_wave(void);
    void emit_seek(size_t tick);

    class vcd_session *session_ = nullptr;
    bool scrubbing_ = false;
    bool scrubbing_wave_ = false;
    int hover_x_ = -1;
    int hover_y_ = -1;

    std::vector<cached_wave_entry> pinned_;
    cached_wave_entry hovered_;
    bool has_hovered_ = false;
    vcd_session::wave_data clk_wave_;
    bool clk_wave_loaded_ = false;
    std::function<void()> on_pinned_changed_;
    size_t last_observed_tick_ = 0;
    uint64_t last_tick_ms_ = 0;
    bool was_playing_ = false;
    /* Flat-top lens width in clock ticks (0 = linear full-VCD view). */
    size_t lens_ticks_ = 0;
    double frozen_lens_center_u_ = 0.5;
};
