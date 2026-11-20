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

#include <SDL2/SDL.h>
#include <cstddef>
#include <string>

class vcd_session;

/*
 * Native SDL2 docked timeline bar drawn at the bottom of a canvas when a VCD
 * session is active. Provides transport buttons, click/drag tick scrubbing,
 * tick/timestamp readout, and a VCD lock/unlock toggle.
 */
class timelinebar
{
public:
    static constexpr int BAR_HEIGHT = 52;

    timelinebar() = default;

    void set_session(class vcd_session *sess) { session_ = sess; }
    bool is_active(void) const;

    void draw(class basecanvas *canvas, const SDL_Rect &bar_rect);

    /* Returns true if the event fell inside the timeline bar and was consumed. */
    bool handle_event(const SDL_Event &event, const SDL_Rect &bar_rect);

    /* Maps a screen X coordinate inside the scrubber track to a clamped tick index. */
    size_t tick_from_x(int screen_x, const SDL_Rect &bar_rect) const;

private:
    struct layout {
        SDL_Rect btn_start{};
        SDL_Rect btn_prev{};
        SDL_Rect btn_play{};
        SDL_Rect btn_next{};
        SDL_Rect btn_end{};
        SDL_Rect track{};
        SDL_Rect readout{};
        SDL_Rect btn_lock{};
    };

    layout compute_layout(const SDL_Rect &bar_rect) const;
    void emit_seek(size_t tick);

    class vcd_session *session_ = nullptr;
    bool scrubbing_ = false;
    int hover_x_ = -1;
    int hover_y_ = -1;
};
