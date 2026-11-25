/*
 * gridcad -- a tool to do digital circuit design with a GUI
 *
 * (C) Copyright 2025  -- Arjan van de Ven
 *
 * Licensed under the terms of the
 * GNU General Public LIcense version 3.0
 */

#include "timelinebar.h"
#include "label_table.h"
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

void draw_clipped_label(SDL_Renderer *ren, class basecanvas *canvas,
                        std::string_view text, const SDL_Rect &box,
                        bool center_x = false)
{
    if (!ren || !canvas || text.empty() || box.w < 6 || box.h < 6)
        return;
    SDL_Texture *tex = canvas->text_to_texture(text);
    if (!tex)
        return;
    float tw = 0.0f, th = 0.0f;
    SDL_GetTextureSize(tex, &tw, &th);
    if (tw <= 0.0f || th <= 0.0f)
        return;

    const float target_h = std::min(static_cast<float>(box.h), 15.0f);
    const float scale = target_h / th;
    const float draw_w = tw * scale;
    const float draw_h = target_h;

    float dx = static_cast<float>(box.x);
    if (center_x && draw_w < static_cast<float>(box.w))
        dx += (static_cast<float>(box.w) - draw_w) * 0.5f;
    const float dy = static_cast<float>(box.y) + (static_cast<float>(box.h) - draw_h) * 0.5f;

    SDL_SetRenderClipRect(ren, &box);
    SDL_FRect dst = {dx, dy, draw_w, draw_h};
    SDL_RenderTexture(ren, tex, nullptr, &dst);
    SDL_SetRenderClipRect(ren, nullptr);
}

std::string format_segment_label(const vcd_session::wave_segment &seg,
                                 int bit_width,
                                 const std::shared_ptr<const class label_table> &labels)
{
    if (seg.val.is_error)
        return "X";
    if (!seg.val.valid)
        return "Z";
    if (bit_width <= 1)
        return seg.val.boolval ? "1" : "0";
    if (labels) {
        const std::string lbl = labels->lookup(seg.val.intval);
        if (!lbl.empty())
            return lbl;
    }
    const int hex_digits = std::max(1, (bit_width + 3) / 4);
    return std::format("{:0{}X}", seg.val.intval, hex_digits);
}

size_t choose_tick_step(size_t visible_ticks, int wave_width_px)
{
    if (visible_ticks <= 1 || wave_width_px <= 0)
        return 1;
    /* Keep at least ~42 px between tick labels so numbers stay crisp inside the clk wave. */
    const size_t max_labels = static_cast<size_t>(std::max(2, wave_width_px / 42));
    const size_t raw_step = (visible_ticks + max_labels - 1) / max_labels;
    if (raw_step <= 1)
        return 1;
    size_t mag = 1;
    while (mag * 10 <= raw_step)
        mag *= 10;
    if (raw_step <= mag)
        return mag;
    if (raw_step <= mag * 2)
        return mag * 2;
    if (raw_step <= mag * 5)
        return mag * 5;
    return mag * 10;
}

void draw_wave_trace(SDL_Renderer *ren, class basecanvas *canvas,
                     const SDL_Rect &wave_rect,
                     const vcd_session::wave_data &data,
                     const std::vector<std::string> *seg_labels,
                     uint64_t t0, uint64_t t1,
                     bool is_clk_row)
{
    if (!ren || data.segments.empty() || t1 <= t0 || wave_rect.w <= 4 || wave_rect.h <= 4)
        return;

    const double inv_span = 1.0 / static_cast<double>(t1 - t0);
    auto time_to_x = [&](uint64_t t) -> float {
        if (t <= t0)
            return static_cast<float>(wave_rect.x);
        if (t >= t1)
            return static_cast<float>(wave_rect.x + wave_rect.w);
        const double f = static_cast<double>(t - t0) * inv_span;
        return static_cast<float>(wave_rect.x) + static_cast<float>(f * static_cast<double>(wave_rect.w));
    };

    const float pad_y = is_clk_row ? 3.0f : 5.0f;
    const float y_top = static_cast<float>(wave_rect.y) + pad_y;
    const float y_bot = static_cast<float>(wave_rect.y + wave_rect.h) - pad_y;
    const float y_mid = 0.5f * (y_top + y_bot);

    /* Find first segment overlapping [t0, t1]. */
    auto it = std::lower_bound(
        data.segments.begin(), data.segments.end(), t0,
        [](const vcd_session::wave_segment &s, uint64_t t) { return s.t_end <= t; });
    size_t idx = static_cast<size_t>(std::distance(data.segments.begin(), it));

    SDL_SetRenderClipRect(ren, &wave_rect);

    const bool is_bus = (data.bit_width > 1);
    bool have_prev_1bit = false;
    float prev_y = y_bot;
    bool in_dense = false;
    float dense_x0 = 0.0f;
    float dense_x1 = 0.0f;

    auto flush_dense = [&]() {
        if (!in_dense)
            return;
        SDL_FRect drect = {dense_x0, y_top, std::max(1.0f, dense_x1 - dense_x0), y_bot - y_top};
        SDL_SetRenderDrawColor(ren, 0, 140, 55, 115);
        SDL_RenderFillRect(ren, &drect);
        SDL_SetRenderDrawColor(ren, 0, 210, 80, 210);
        SDL_RenderRect(ren, &drect);
        in_dense = false;
        have_prev_1bit = false;
    };

    for (; idx < data.segments.size(); ++idx) {
        const auto &seg = data.segments[idx];
        if (seg.t_start >= t1)
            break;
        const float x0 = time_to_x(seg.t_start);
        const float x1 = time_to_x(seg.t_end);
        const float seg_w = x1 - x0;
        if (seg_w <= 0.0f)
            continue;

        /* Collapse sub-2px transition bursts into a single dense band for 60 FPS on huge VCDs. */
        if (seg_w < 2.0f) {
            if (!in_dense) {
                in_dense = true;
                dense_x0 = x0;
            }
            dense_x1 = x1;
            continue;
        }
        flush_dense();

        const bool is_x = seg.val.is_error;
        const bool is_z = !seg.val.valid && !seg.val.is_error;
        const bool qual_invalid = seg.has_qualifier && !seg.val.qualifier_valid;
        const bool qual_valid = seg.has_qualifier && seg.val.qualifier_valid;

        if (!is_bus) {
            if (is_x) {
                SDL_FRect r = {x0, y_top, seg_w, y_bot - y_top};
                SDL_SetRenderDrawColor(ren, 175, 40, 40, 140);
                SDL_RenderFillRect(ren, &r);
                SDL_SetRenderDrawColor(ren, 230, 70, 70, 240);
                SDL_RenderLine(ren, x0, y_mid, x1, y_mid);
                have_prev_1bit = false;
            } else if (is_z) {
                SDL_SetRenderDrawColor(ren, 220, 180, 45, 240);
                SDL_RenderLine(ren, x0, y_mid, x1, y_mid);
                have_prev_1bit = false;
            } else {
                const float y = seg.val.boolval ? y_top : y_bot;
                if (seg.val.boolval && !is_clk_row) {
                    SDL_FRect fill = {x0, y_top, seg_w, y_bot - y_top};
                    if (qual_invalid)
                        SDL_SetRenderDrawColor(ren, 60, 80, 70, 28);
                    else
                        SDL_SetRenderDrawColor(ren, 0, 190, 70, 34);
                    SDL_RenderFillRect(ren, &fill);
                }
                if (qual_invalid)
                    SDL_SetRenderDrawColor(ren, 85, 115, 95, 190);
                else if (is_clk_row)
                    SDL_SetRenderDrawColor(ren, 0, 195, 75, 220);
                else
                    SDL_SetRenderDrawColor(ren, 20, 235, 90, 255);

                if (have_prev_1bit && std::abs(prev_y - y) > 0.5f)
                    SDL_RenderLine(ren, x0, prev_y, x0, y);
                SDL_RenderLine(ren, x0, y, x1, y);
                prev_y = y;
                have_prev_1bit = true;
            }
        } else {
            /* Multi-bit GTKWave-style hexagonal bus envelope. */
            const float slant = std::min(4.0f, seg_w * 0.25f);
            Uint8 fr = 0, fg = 70, fb = 25, fa = 45;
            Uint8 lr = 0, lg = 215, lb = 80, la = 255;
            if (is_x) {
                fr = 145; fg = 28; fb = 28; fa = 120;
                lr = 230; lg = 65; lb = 65; la = 255;
            } else if (is_z) {
                fr = 125; fg = 95; fb = 20; fa = 85;
                lr = 220; lg = 180; lb = 45; la = 255;
            } else if (qual_invalid) {
                fr = 28; fg = 34; fb = 32; fa = 90;
                lr = 78; lg = 98; lb = 88; la = 190;
            } else if (qual_valid) {
                fr = 0; fg = 125; fb = 48; fa = 85;
                lr = 35; lg = 245; lb = 105; la = 255;
            }

            SDL_FRect inner = {x0 + slant, y_top, std::max(1.0f, seg_w - 2.0f * slant), y_bot - y_top};
            SDL_SetRenderDrawColor(ren, fr, fg, fb, fa);
            SDL_RenderFillRect(ren, &inner);

            SDL_SetRenderDrawColor(ren, lr, lg, lb, la);
            SDL_RenderLine(ren, x0 + slant, y_top, x1 - slant, y_top);
            SDL_RenderLine(ren, x0 + slant, y_bot, x1 - slant, y_bot);
            SDL_RenderLine(ren, x0, y_mid, x0 + slant, y_top);
            SDL_RenderLine(ren, x0, y_mid, x0 + slant, y_bot);
            SDL_RenderLine(ren, x1 - slant, y_top, x1, y_mid);
            SDL_RenderLine(ren, x1 - slant, y_bot, x1, y_mid);

            if (seg_labels && idx < seg_labels->size() && seg_w >= 18.0f) {
                SDL_Rect tbox = {
                    static_cast<int>(std::lround(x0 + slant + 3.0f)),
                    static_cast<int>(std::lround(y_top + 1.0f)),
                    std::max(1, static_cast<int>(std::lround(seg_w - 2.0f * slant - 6.0f))),
                    std::max(1, static_cast<int>(std::lround(y_bot - y_top - 2.0f)))
                };
                /* Clip to intersection with wave_rect so labels never spill outside the wave pane. */
                const int rx0 = std::max(tbox.x, wave_rect.x);
                const int rx1 = std::min(tbox.x + tbox.w, wave_rect.x + wave_rect.w);
                if (rx1 - rx0 >= 10) {
                    SDL_Rect clipped = {rx0, tbox.y, rx1 - rx0, tbox.h};
                    draw_clipped_label(ren, canvas, (*seg_labels)[idx], clipped, true);
                    SDL_SetRenderClipRect(ren, &wave_rect);
                }
            }
        }
    }
    flush_dense();
    SDL_SetRenderClipRect(ren, nullptr);
}

} // namespace

void timelinebar::set_session(class vcd_session *sess)
{
    if (session_ == sess)
        return;
    session_ = sess;
    pinned_.clear();
    has_hovered_ = false;
    hovered_ = {};
    clk_wave_ = {};
    clk_wave_loaded_ = false;
}

bool timelinebar::is_active(void) const
{
    return session_ != nullptr && session_->is_loaded() && session_->get_total_ticks() > 0;
}

void timelinebar::ensure_clock_wave(void)
{
    if (clk_wave_loaded_ || !session_ || !session_->is_loaded())
        return;
    clk_wave_ = session_->extract_clock_wave();
    clk_wave_loaded_ = true;
}

void timelinebar::rebuild_entry(cached_wave_entry &entry, const wave_signal_ref &sig, bool pinned)
{
    entry.ref = sig;
    entry.pinned = pinned;
    entry.data = session_ ? session_->extract_wave(sig.vcd_name) : vcd_session::wave_data{};
    if (entry.ref.display_name.empty())
        entry.ref.display_name = entry.data.signal_name.empty() ? sig.vcd_name : entry.data.signal_name;
    const int bw = std::max(entry.data.bit_width, sig.bit_width);
    entry.data.bit_width = bw;
    entry.seg_labels.clear();
    entry.seg_labels.reserve(entry.data.segments.size());
    for (const auto &seg : entry.data.segments)
        entry.seg_labels.push_back(format_segment_label(seg, bw, sig.labels));
}

void timelinebar::set_hovered_signal(const wave_signal_ref &sig)
{
    if (!is_active() || sig.vcd_name.empty() || !session_->has_binding(sig.vcd_name)) {
        clear_hovered_signal();
        return;
    }
    if (has_hovered_ && hovered_.ref.vcd_name == sig.vcd_name && hovered_.ref.labels == sig.labels)
        return;
    ensure_clock_wave();
    rebuild_entry(hovered_, sig, false);
    has_hovered_ = !hovered_.data.segments.empty();
}

void timelinebar::clear_hovered_signal(void)
{
    has_hovered_ = false;
    hovered_.ref.vcd_name.clear();
}

bool timelinebar::is_pinned(std::string_view vcd_name) const
{
    for (const auto &p : pinned_) {
        if (p.ref.vcd_name == vcd_name)
            return true;
    }
    return false;
}

void timelinebar::pin_signal(const wave_signal_ref &sig)
{
    if (!is_active() || sig.vcd_name.empty() || !session_->has_binding(sig.vcd_name))
        return;
    if (is_pinned(sig.vcd_name))
        return;
    if (pinned_.size() >= MAX_PINNED)
        pinned_.erase(pinned_.begin());
    ensure_clock_wave();
    cached_wave_entry entry;
    rebuild_entry(entry, sig, true);
    if (!entry.data.segments.empty())
        pinned_.push_back(std::move(entry));
}

void timelinebar::unpin_signal(std::string_view vcd_name)
{
    std::erase_if(pinned_, [&](const cached_wave_entry &e) {
        return e.ref.vcd_name == vcd_name;
    });
}

void timelinebar::clear_pinned_signals(void)
{
    pinned_.clear();
}

bool timelinebar::has_active_wave(void) const
{
    if (!is_active())
        return false;
    return !pinned_.empty() || has_hovered_;
}

bool timelinebar::has_unpinned_hover(void) const
{
    return has_hovered_ && !is_pinned(hovered_.ref.vcd_name);
}

int timelinebar::get_height(void) const
{
    if (!has_active_wave())
        return BAR_HEIGHT;
    const size_t rows = pinned_.size() + (has_unpinned_hover() ? 1 : 0);
    return BAR_HEIGHT + CLK_RULER_HEIGHT + static_cast<int>(rows) * WAVE_ROW_HEIGHT + 4;
}

std::pair<uint64_t, uint64_t> timelinebar::compute_time_window(void) const
{
    if (!session_)
        return {0, 1};
    const size_t total = session_->get_total_ticks();
    const uint64_t t_min = session_->get_start_time();
    uint64_t t_max = session_->get_end_time();
    if (total > 0)
        t_max = std::max(t_max, session_->get_tick_timestamp(total - 1));
    if (t_max <= t_min)
        t_max = t_min + 1;

    if (total <= 64)
        return {t_min, t_max};

    const size_t half = 24;
    const size_t cur = session_->get_current_tick();
    size_t i0 = (cur > half) ? (cur - half) : 0;
    size_t i1 = std::min(total - 1, i0 + 2 * half);
    if (i1 - i0 < 2 * half && i1 >= 2 * half)
        i0 = i1 - 2 * half;

    uint64_t w0 = session_->get_tick_timestamp(i0);
    uint64_t w1 = session_->get_tick_timestamp(i1);
    if (i1 + 1 < total) {
        const uint64_t next_t = session_->get_tick_timestamp(i1 + 1);
        w1 += (next_t - w1) / 2;
    }
    if (w1 <= w0)
        w1 = w0 + 1;
    return {w0, w1};
}

timelinebar::layout timelinebar::compute_layout(const SDL_Rect &bar_rect) const
{
    layout l{};
    const int controls_y = bar_rect.y + bar_rect.h - BAR_HEIGHT;
    l.controls_rect = {bar_rect.x, controls_y, bar_rect.w, BAR_HEIGHT};

    const int pad = 8;
    const int btn_w = 34;
    const int btn_h = BAR_HEIGHT - 16;
    const int btn_y = controls_y + 8;
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
    l.track = {track_x, controls_y + 10, track_w, BAR_HEIGHT - 20};

    if (has_active_wave() && bar_rect.h > BAR_HEIGHT) {
        const int wave_h = controls_y - bar_rect.y;
        l.wave_panel = {bar_rect.x, bar_rect.y, bar_rect.w, wave_h};
        const int sig_col = std::min(SIGNAL_COL_WIDTH, std::max(90, bar_rect.w / 4));
        const int wave_x = bar_rect.x + sig_col;
        const int wave_w = std::max(40, bar_rect.w - sig_col - pad);

        /*
         * Vertical layout (top -> bottom):
         *   [optional live hover row] -> [clk + tick ruler] -> [pinned rows] -> [controls]
         * Keeping the hover row at the top means pinned signals and the clk ruler never shift
         * vertically as the mouse sweeps across wires.
         */
        int y = bar_rect.y + 2;
        const int wave_top_y = y;

        if (has_unpinned_hover()) {
            l.hover_row = {wave_x, y, wave_w, WAVE_ROW_HEIGHT};
            y += WAVE_ROW_HEIGHT;
        }

        l.clk_row = {wave_x, y, wave_w, CLK_RULER_HEIGHT};
        y += CLK_RULER_HEIGHT;

        l.pinned_rows.reserve(pinned_.size());
        l.unpin_btns.reserve(pinned_.size());
        for (size_t i = 0; i < pinned_.size(); ++i) {
            l.pinned_rows.push_back({wave_x, y, wave_w, WAVE_ROW_HEIGHT});
            l.unpin_btns.push_back({bar_rect.x + 6, y + 7, 20, 20});
            y += WAVE_ROW_HEIGHT;
        }
        l.wave_area = {wave_x, wave_top_y, wave_w, std::max(1, y - wave_top_y)};
    }
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

size_t timelinebar::tick_from_wave_x(int screen_x, const SDL_Rect &wave_area) const
{
    if (!session_ || session_->get_total_ticks() <= 1)
        return 0;
    const auto [t0, t1] = compute_time_window();
    const double rel = static_cast<double>(screen_x - wave_area.x) /
                       static_cast<double>(std::max(1, wave_area.w));
    const double clamped = std::clamp(rel, 0.0, 1.0);
    const uint64_t target_t = t0 + static_cast<uint64_t>(std::llround(clamped * static_cast<double>(t1 - t0)));
    return session_->find_nearest_tick(target_t);
}

void timelinebar::emit_seek(size_t tick)
{
    if (session_)
        session_->set_unlocked(false);
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

    if (event.type == SDL_EVENT_MOUSE_MOTION) {
        hover_x_ = static_cast<int>(event.motion.x);
        hover_y_ = static_cast<int>(event.motion.y);
        if (scrubbing_) {
            emit_seek(tick_from_x(hover_x_, bar_rect));
            return true;
        }
        if (scrubbing_wave_) {
            emit_seek(tick_from_wave_x(hover_x_, l.wave_area));
            return true;
        }
        return point_in_rect(hover_x_, hover_y_, bar_rect);
    }

    if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT) {
        if (scrubbing_ || scrubbing_wave_) {
            scrubbing_ = false;
            scrubbing_wave_ = false;
            return true;
        }
        return point_in_rect(static_cast<int>(event.button.x), static_cast<int>(event.button.y), bar_rect);
    }

    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
        const int mx = static_cast<int>(event.button.x);
        const int my = static_cast<int>(event.button.y);
        if (!point_in_rect(mx, my, bar_rect))
            return false;

        /* Check per-row unpin buttons first. */
        for (size_t i = 0; i < l.unpin_btns.size() && i < pinned_.size(); ++i) {
            if (point_in_rect(mx, my, l.unpin_btns[i])) {
                pinned_.erase(pinned_.begin() + static_cast<std::ptrdiff_t>(i));
                return true;
            }
        }

        /* Clicking inside the waveform area scrubs to the clicked timestamp. */
        if (has_active_wave() && point_in_rect(mx, my, l.wave_area)) {
            scrubbing_wave_ = true;
            emit_seek(tick_from_wave_x(mx, l.wave_area));
            return true;
        }

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
            if (!session_->is_unlocked()) {
                session_->set_unlocked(true);
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
    SDL_Renderer *ren = canvas->get_renderer();

    /* 1. GTKWave-style wave viewer pane above the transport controls (when hovering or pinned). */
    if (has_active_wave() && ren && l.wave_panel.h > 0) {
        canvas->flush_line_batch();
        const auto [t0, t1] = compute_time_window();
        const double inv_span = 1.0 / static_cast<double>(std::max<uint64_t>(1, t1 - t0));

        /* Deep GTKWave dark background + top divider border. */
        SDL_FRect panel_f = {static_cast<float>(l.wave_panel.x), static_cast<float>(l.wave_panel.y),
                             static_cast<float>(l.wave_panel.w), static_cast<float>(l.wave_panel.h)};
        SDL_SetRenderDrawColor(ren, 10, 12, 16, 245);
        SDL_RenderFillRect(ren, &panel_f);

        /* Left signal-name column subtle tint + vertical separator. */
        const int sig_col_w = l.wave_area.x - l.wave_panel.x;
        SDL_FRect sig_bg = {static_cast<float>(l.wave_panel.x), static_cast<float>(l.wave_panel.y),
                            static_cast<float>(sig_col_w), static_cast<float>(l.wave_panel.h)};
        SDL_SetRenderDrawColor(ren, 20, 24, 30, 245);
        SDL_RenderFillRect(ren, &sig_bg);

        SDL_SetRenderDrawColor(ren, 80, 92, 108, 220);
        SDL_RenderLine(ren, panel_f.x, panel_f.y, panel_f.x + panel_f.w, panel_f.y);
        SDL_RenderLine(ren, static_cast<float>(l.wave_area.x - 1), panel_f.y,
                       static_cast<float>(l.wave_area.x - 1), panel_f.y + panel_f.h);

        /* Clock-tick vertical gridlines across the entire wave area. */
        const size_t total_ticks = session_->get_total_ticks();
        const size_t first_vis_tick = session_->find_nearest_tick(t0);
        const size_t last_vis_tick = session_->find_nearest_tick(t1);
        const size_t vis_ticks = (last_vis_tick >= first_vis_tick) ? (last_vis_tick - first_vis_tick + 1) : 1;
        const size_t tick_step = choose_tick_step(vis_ticks, l.wave_area.w);
        const size_t first_grid_tick = ((first_vis_tick + tick_step - 1) / tick_step) * tick_step;

        for (size_t tk = first_grid_tick; tk < total_ticks; tk += tick_step) {
            const uint64_t tt = session_->get_tick_timestamp(tk);
            if (tt < t0)
                continue;
            if (tt > t1)
                break;
            const float gx = static_cast<float>(l.wave_area.x) +
                             static_cast<float>(static_cast<double>(tt - t0) * inv_span * static_cast<double>(l.wave_area.w));
            SDL_SetRenderDrawColor(ren, 38, 46, 58, 170);
            SDL_RenderLine(ren, gx, static_cast<float>(l.wave_area.y),
                           gx, static_cast<float>(l.wave_area.y + l.wave_area.h));
        }

        auto draw_signal_row = [&](const cached_wave_entry &e, const SDL_Rect &row, const SDL_Rect *unpin_btn) {
            if (!e.pinned) {
                SDL_FRect hbg = {static_cast<float>(l.wave_panel.x), static_cast<float>(row.y),
                                 static_cast<float>(sig_col_w - 1), static_cast<float>(row.h)};
                SDL_SetRenderDrawColor(ren, 28, 38, 48, 225);
                SDL_RenderFillRect(ren, &hbg);
            } else if (unpin_btn) {
                const bool uhov = point_in_rect(hover_x_, hover_y_, *unpin_btn);
                SDL_FRect ubf = {static_cast<float>(unpin_btn->x), static_cast<float>(unpin_btn->y),
                                 static_cast<float>(unpin_btn->w), static_cast<float>(unpin_btn->h)};
                SDL_SetRenderDrawColor(ren, uhov ? 165 : 55, uhov ? 50 : 62, uhov ? 50 : 72, 230);
                SDL_RenderFillRect(ren, &ubf);
                draw_clipped_label(ren, canvas, "x", *unpin_btn, true);
            }

            const int lbl_x = e.pinned ? (l.wave_panel.x + 30) : (l.wave_panel.x + 8);
            const std::string title = (e.data.bit_width > 1)
                ? std::format("{}[{}:0]", e.ref.display_name, e.data.bit_width - 1)
                : e.ref.display_name;
            SDL_Rect lbl_box = {lbl_x, row.y + 6, std::max(1, l.wave_area.x - lbl_x - 6), row.h - 12};
            draw_clipped_label(ren, canvas, title, lbl_box, false);

            draw_wave_trace(ren, canvas, row, e.data, &e.seg_labels, t0, t1, false);

            SDL_SetRenderDrawColor(ren, 42, 48, 58, 200);
            SDL_RenderLine(ren, panel_f.x, static_cast<float>(row.y + row.h),
                           panel_f.x + panel_f.w, static_cast<float>(row.y + row.h));
        };

        /* Live hover row at the top (so pinned rows & clk ruler never shift vertically). */
        if (has_unpinned_hover() && l.hover_row.h > 0)
            draw_signal_row(hovered_, l.hover_row, nullptr);

        /* Unified clk wave + clock-tick numbers row. */
        if (l.clk_row.h > 0) {
            SDL_Rect clk_lbl = {l.wave_panel.x + 8, l.clk_row.y + 3, std::max(1, sig_col_w - 14), CLK_RULER_HEIGHT - 6};
            const std::string clk_title = clk_wave_.signal_name.empty() ? "ticks" : clk_wave_.signal_name;
            draw_clipped_label(ren, canvas, clk_title, clk_lbl, false);

            if (!clk_wave_.segments.empty())
                draw_wave_trace(ren, canvas, l.clk_row, clk_wave_, nullptr, t0, t1, true);

            /* Overlay clock tick numbers directly inside the clk wave row. */
            for (size_t tk = first_grid_tick; tk < total_ticks; tk += tick_step) {
                const uint64_t tt = session_->get_tick_timestamp(tk);
                if (tt < t0)
                    continue;
                if (tt > t1)
                    break;
                const float gx = static_cast<float>(l.wave_area.x) +
                                 static_cast<float>(static_cast<double>(tt - t0) * inv_span * static_cast<double>(l.wave_area.w));
                const int lx = static_cast<int>(std::lround(gx)) + 2;
                const int avail = (l.wave_area.x + l.wave_area.w) - lx;
                if (avail < 14)
                    continue;
                const std::string tlbl = std::format("{}", tk);
                const int pill_w = std::min(avail, std::max(16, static_cast<int>(tlbl.size()) * 8 + 6));
                SDL_FRect pill = {static_cast<float>(lx), static_cast<float>(l.clk_row.y + 3),
                                  static_cast<float>(pill_w), static_cast<float>(CLK_RULER_HEIGHT - 6)};
                SDL_SetRenderDrawColor(ren, 10, 12, 16, 195);
                SDL_RenderFillRect(ren, &pill);
                SDL_Rect rlbl = {lx + 2, l.clk_row.y + 3, std::max(1, pill_w - 4), CLK_RULER_HEIGHT - 6};
                draw_clipped_label(ren, canvas, tlbl, rlbl, false);
            }

            SDL_SetRenderDrawColor(ren, 55, 65, 78, 220);
            SDL_RenderLine(ren, panel_f.x, static_cast<float>(l.clk_row.y + l.clk_row.h),
                           panel_f.x + panel_f.w, static_cast<float>(l.clk_row.y + l.clk_row.h));
        }

        /* Pinned signal rows anchored at the bottom above transport controls. */
        for (size_t i = 0; i < pinned_.size() && i < l.pinned_rows.size(); ++i) {
            const SDL_Rect *ub = (i < l.unpin_btns.size()) ? &l.unpin_btns[i] : nullptr;
            draw_signal_row(pinned_[i], l.pinned_rows[i], ub);
        }

        /* Vertical playhead cursor across the entire wave area. */
        const uint64_t cur_t = session_->get_current_timestamp();
        if (cur_t >= t0 && cur_t <= t1) {
            const float px = static_cast<float>(l.wave_area.x) +
                             static_cast<float>(static_cast<double>(cur_t - t0) * inv_span * static_cast<double>(l.wave_area.w));
            SDL_SetRenderDrawColor(ren, 235, 245, 255, 230);
            SDL_RenderLine(ren, px, static_cast<float>(l.wave_area.y),
                           px, static_cast<float>(l.wave_area.y + l.wave_area.h));
            SDL_RenderLine(ren, px + 1.0f, static_cast<float>(l.wave_area.y),
                           px + 1.0f, static_cast<float>(l.wave_area.y + l.wave_area.h));
        }
    }

    /* 2. Docked dark translucent transport control strip at the bottom. */
    draw_screen_box(canvas, l.controls_rect, COLOR_BACKGROUND_MAIN, 235);
    SDL_Rect top_border = {l.controls_rect.x, l.controls_rect.y, l.controls_rect.w, 2};
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
    const bool unlocked = session_->is_unlocked();
    draw_btn(l.btn_lock, unlocked ? "SIM" : "VCD", !unlocked);
}
