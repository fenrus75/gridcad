/*
 * gridcad -- a tool to do digital circuit design with a GUI
 *
 * (C) Copyright 2025  -- Arjan van de Ven
 *
 * Licensed under the terms of the
 * GNU General Public LIcense version 3.0
 */

#include "timelinebar.h"
#include "vcd_session.h"

#include <algorithm>
#include <cmath>
#include <format>

extern bool clock_running;

namespace {

bool point_in_rect(int x, int y, const SDL_Rect &r)
{
    return x >= r.x && x < (r.x + r.w) && y >= r.y && y < (r.y + r.h);
}

void draw_screen_box(class basecanvas *canvas, const SDL_Rect &r, int color, int alpha = 255)
{
    canvas->draw_box(canvas->scr_to_X(r.x), canvas->scr_to_Y(r.y),
                     canvas->scr_to_X(r.x + r.w), canvas->scr_to_Y(r.y + r.h),
                     color, alpha);
}

void draw_screen_text(class basecanvas *canvas, std::string_view text, const SDL_Rect &r, int pad = 4)
{
    const int x = r.x + pad;
    const int y = r.y + pad;
    const int w = std::max(1, r.w - 2 * pad);
    const int h = std::max(1, r.h - 2 * pad);
    canvas->draw_text(text, canvas->scr_to_X(x), canvas->scr_to_Y(y),
                      canvas->scale_to_X(w), canvas->scale_to_Y(h));
}

} // namespace

bool timelinebar::is_active(void) const
{
    return session_ != nullptr && session_->is_loaded() && session_->get_total_ticks() > 0;
}

timelinebar::layout timelinebar::compute_layout(const SDL_Rect &bar_rect) const
{
    layout l{};
    const int pad = 8;
    const int btn_w = 34;
    const int btn_h = bar_rect.h - 16;
    const int btn_y = bar_rect.y + 8;
    const int gap = 4;

    int x = bar_rect.x + pad;
    l.btn_start = {x, btn_y, btn_w, btn_h}; x += btn_w + gap;
    l.btn_prev  = {x, btn_y, btn_w, btn_h}; x += btn_w + gap;
    l.btn_play  = {x, btn_y, btn_w + 6, btn_h}; x += btn_w + 6 + gap;
    l.btn_next  = {x, btn_y, btn_w, btn_h}; x += btn_w + gap;
    l.btn_end   = {x, btn_y, btn_w, btn_h}; x += btn_w + pad + 4;

    const int lock_w = 68;
    const int readout_w = 210;
    const int right_edge = bar_rect.x + bar_rect.w - pad;

    l.btn_lock = {right_edge - lock_w, btn_y, lock_w, btn_h};
    l.readout  = {l.btn_lock.x - gap - readout_w, btn_y, readout_w, btn_h};

    const int track_x = x;
    const int track_w = std::max(40, l.readout.x - pad - track_x);
    l.track = {track_x, bar_rect.y + 10, track_w, bar_rect.h - 20};
    return l;
}

size_t timelinebar::tick_from_x(int screen_x, const SDL_Rect &bar_rect) const
{
    if (!session_ || session_->get_total_ticks() <= 1)
        return 0;

    const layout l = compute_layout(bar_rect);
    const double rel = static_cast<double>(screen_x - l.track.x) /
                       static_cast<double>(std::max(1, l.track.w));
    const double clamped = std::clamp(rel, 0.0, 1.0);
    const size_t max_tick = session_->get_total_ticks() - 1;
    return static_cast<size_t>(std::llround(clamped * static_cast<double>(max_tick)));
}

void timelinebar::emit_seek(size_t tick)
{
    vcd_unlocked_ = false;
    SDL_Event ev = {};
    ev.type = EVENT_VCD_SEEK;
    ev.user.code = static_cast<Sint32>(tick);
    SDL_PushEvent(&ev);
}

bool timelinebar::handle_event(const SDL_Event &event, const SDL_Rect &bar_rect)
{
    if (!is_active())
        return false;

    const layout l = compute_layout(bar_rect);

    if (event.type == SDL_MOUSEMOTION) {
        hover_x_ = event.motion.x;
        hover_y_ = event.motion.y;
        if (scrubbing_) {
            emit_seek(tick_from_x(event.motion.x, bar_rect));
            return true;
        }
        return point_in_rect(event.motion.x, event.motion.y, bar_rect);
    }

    if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
        if (scrubbing_) {
            scrubbing_ = false;
            return true;
        }
        return point_in_rect(event.button.x, event.button.y, bar_rect);
    }

    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        const int mx = event.button.x;
        const int my = event.button.y;
        if (!point_in_rect(mx, my, bar_rect))
            return false;

        const size_t cur = session_->get_current_tick();
        const size_t total = session_->get_total_ticks();

        if (point_in_rect(mx, my, l.btn_start)) {
            emit_seek(0);
        } else if (point_in_rect(mx, my, l.btn_prev)) {
            emit_seek(cur > 0 ? cur - 1 : total - 1);
        } else if (point_in_rect(mx, my, l.btn_play)) {
            SDL_Event ev = {};
            ev.type = clock_running ? EVENT_STOP_CLOCK : EVENT_START_CLOCK;
            SDL_PushEvent(&ev);
        } else if (point_in_rect(mx, my, l.btn_next)) {
            emit_seek((cur + 1 < total) ? cur + 1 : 0);
        } else if (point_in_rect(mx, my, l.btn_end)) {
            emit_seek(total - 1);
        } else if (point_in_rect(mx, my, l.btn_lock)) {
            if (!vcd_unlocked_) {
                vcd_unlocked_ = true;
                SDL_Event ev = {};
                ev.type = EVENT_VCD_UNLOCK;
                SDL_PushEvent(&ev);
            } else {
                emit_seek(cur);
            }
        } else if (point_in_rect(mx, my, l.track)) {
            scrubbing_ = true;
            emit_seek(tick_from_x(mx, bar_rect));
        }
        return true;
    }

    return false;
}

void timelinebar::draw(class basecanvas *canvas, const SDL_Rect &bar_rect)
{
    if (!is_active() || !canvas)
        return;

    const layout l = compute_layout(bar_rect);

    /* Docked dark translucent bar background + top divider border. */
    draw_screen_box(canvas, bar_rect, COLOR_BACKGROUND_MAIN, 235);
    SDL_Rect top_border = {bar_rect.x, bar_rect.y, bar_rect.w, 2};
    draw_screen_box(canvas, top_border, COLOR_WIRE_SOLID, 220);

    auto draw_btn = [&](const SDL_Rect &r, std::string_view label, bool active_highlight = false) {
        const bool hovered = point_in_rect(hover_x_, hover_y_, r);
        const int bg = active_highlight ? COLOR_VALUE_GREEN
                                        : (hovered ? COLOR_WIRE_SOLID : COLOR_BACKGROUND_GRID);
        draw_screen_box(canvas, r, bg, 230);
        draw_screen_text(canvas, label, r, 5);
    };

    draw_btn(l.btn_start, "|<");
    draw_btn(l.btn_prev, "<");
    draw_btn(l.btn_play, clock_running ? "||" : "Play", clock_running);
    draw_btn(l.btn_next, ">");
    draw_btn(l.btn_end, ">|");

    /* Scrubber track background. */
    SDL_Rect rail = {l.track.x, l.track.y + l.track.h / 2 - 4, l.track.w, 8};
    draw_screen_box(canvas, rail, COLOR_BACKGROUND_GRID, 255);

    const size_t cur = session_->get_current_tick();
    const size_t total = session_->get_total_ticks();
    const double frac = (total > 1) ? static_cast<double>(cur) / static_cast<double>(total - 1) : 0.0;
    const int fill_w = static_cast<int>(std::llround(frac * static_cast<double>(rail.w)));

    if (fill_w > 0) {
        SDL_Rect filled = {rail.x, rail.y, fill_w, rail.h};
        draw_screen_box(canvas, filled, COLOR_VALUE_GREEN, 220);
    }

    /* Tick notches along the track (cap drawn notches to keep dense VCDs crisp). */
    const size_t max_notches = 64;
    const size_t step = (total > max_notches) ? ((total + max_notches - 1) / max_notches) : 1;
    for (size_t i = 0; i < total; i += step) {
        const double tf = (total > 1) ? static_cast<double>(i) / static_cast<double>(total - 1) : 0.0;
        const int nx = l.track.x + static_cast<int>(std::llround(tf * static_cast<double>(l.track.w)));
        SDL_Rect notch = {nx, rail.y + rail.h + 2, 1, 5};
        draw_screen_box(canvas, notch, COLOR_WIRE_SOLID, 160);
    }

    /* Playhead thumb handle. */
    const int thumb_x = l.track.x + fill_w - 5;
    SDL_Rect thumb = {thumb_x, l.track.y + 2, 10, l.track.h - 4};
    draw_screen_box(canvas, thumb, COLOR_WIRE_MOTION, 255);

    /* Tick + timestamp readout. */
    const std::string info = std::format("Tick {}/{}  t={}{}",
                                         cur, total > 0 ? total - 1 : 0,
                                         session_->get_current_timestamp(),
                                         session_->get_timescale());
    draw_screen_text(canvas, info, l.readout, 6);

    /* VCD lock / free-sim toggle button. */
    draw_btn(l.btn_lock, vcd_unlocked_ ? "SIM" : "VCD", !vcd_unlocked_);
}
