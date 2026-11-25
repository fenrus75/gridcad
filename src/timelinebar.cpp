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
#include <array>
#include <cmath>
#include <format>
#include <numbers>

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

void draw_wave_trace(SDL_Renderer *ren, class basecanvas *canvas,
                     const SDL_Rect &wave_rect,
                     const vcd_session::wave_data &data,
                     const std::vector<std::string> *seg_labels,
                     uint64_t t0, uint64_t t1,
                     const timelinebar::wave_lens &lens,
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
        const double u = static_cast<double>(t - t0) * inv_span;
        const double sx = lens.x_from_u(u);
        return static_cast<float>(wave_rect.x) + static_cast<float>(sx * static_cast<double>(wave_rect.w));
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

            if (!qual_invalid && seg_labels && idx < seg_labels->size() && seg_w >= 18.0f) {
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
    /* Short VCDs (<= 32 ticks) fit cleanly linearly; long VCDs start with a 4-tick flat-top lens. */
    lens_ticks_ = (session_ && session_->get_total_ticks() > 32) ? 4 : 0;
}

double timelinebar::wave_lens::x_from_u(double u) const noexcept
{
    u = std::clamp(u, 0.0, 1.0);
    if (!is_active())
        return u;

    auto kernel_integral = [this](double d) -> double {
        if (d <= 0.0)
            return 0.0;
        if (d <= half_flat)
            return d;
        if (ramp > 1e-12 && d < half_flat + ramp) {
            const double s = d - half_flat;
            return half_flat + 0.5 * s +
                   (ramp / (2.0 * std::numbers::pi)) * std::sin(std::numbers::pi * s / ramp);
        }
        return half_flat + 0.5 * ramp;
    };

    auto F = [&](double v) -> double {
        const double dv = v - center;
        const double ki = kernel_integral(std::abs(dv));
        return dv + boost * ((dv >= 0.0) ? ki : -ki);
    };

    const double f0 = F(0.0);
    const double f1 = F(1.0);
    const double denom = f1 - f0;
    if (denom <= 1e-12)
        return u;
    return std::clamp((F(u) - f0) / denom, 0.0, 1.0);
}

double timelinebar::wave_lens::u_from_x(double x) const noexcept
{
    x = std::clamp(x, 0.0, 1.0);
    if (!is_active())
        return x;

    double lo = 0.0;
    double hi = 1.0;
    for (int i = 0; i < 24; ++i) {
        const double mid = 0.5 * (lo + hi);
        if (x_from_u(mid) < x)
            lo = mid;
        else
            hi = mid;
    }
    return 0.5 * (lo + hi);
}

namespace {
constexpr std::array<size_t, 6> kZoomSteps = {2, 4, 8, 16, 32, 64};
} // namespace

void timelinebar::zoom_in(void)
{
    if (!session_ || session_->get_total_ticks() <= 2)
        return;
    const size_t total = session_->get_total_ticks();
    if (lens_ticks_ == 0) {
        /* Step from linear full-view into the largest preset smaller than total_ticks (or 4). */
        size_t chosen = 4;
        for (size_t s : kZoomSteps) {
            if (s * 2 <= total)
                chosen = s;
        }
        lens_ticks_ = chosen;
        return;
    }
    for (auto it = kZoomSteps.rbegin(); it != kZoomSteps.rend(); ++it) {
        if (*it < lens_ticks_) {
            lens_ticks_ = *it;
            return;
        }
    }
    lens_ticks_ = kZoomSteps.front();
}

void timelinebar::zoom_out(void)
{
    if (lens_ticks_ == 0 || !session_)
        return;
    const size_t total = session_->get_total_ticks();
    for (size_t s : kZoomSteps) {
        if (s > lens_ticks_) {
            lens_ticks_ = (s >= total) ? 0 : s;
            return;
        }
    }
    lens_ticks_ = 0;
}

timelinebar::wave_lens timelinebar::build_lens(double center_u) const noexcept
{
    wave_lens lens;
    lens.center = std::clamp(center_u, 0.0, 1.0);
    if (!session_ || lens_ticks_ == 0)
        return lens;
    const size_t total = session_->get_total_ticks();
    if (total <= 2 || lens_ticks_ >= total)
        return lens;

    const double span_ticks = static_cast<double>(total - 1);
    lens.half_flat = 0.5 * static_cast<double>(lens_ticks_) / span_ticks;
    lens.ramp = lens.half_flat;
    const double lens_area = 2.0 * lens.half_flat + lens.ramp;
    if (lens_area >= 0.95)
        return lens;

    /* Allocate ~58% of the viewport to the magnified lens when zoomed in, fading smoothly to linear. */
    const double extra_share = 0.58 * (1.0 - lens_area);
    lens.boost = (extra_share / std::max(0.05, 1.0 - extra_share)) / lens_area;
    return lens;
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

void timelinebar::pin_signal(const wave_signal_ref &sig, bool notify)
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
    if (!entry.data.segments.empty()) {
        pinned_.push_back(std::move(entry));
        if (notify && on_pinned_changed_)
            on_pinned_changed_();
    }
}

void timelinebar::unpin_signal(std::string_view vcd_name, bool notify)
{
    const size_t removed = std::erase_if(pinned_, [&](const cached_wave_entry &e) {
        return e.ref.vcd_name == vcd_name;
    });
    if (removed > 0 && notify && on_pinned_changed_)
        on_pinned_changed_();
}

void timelinebar::clear_pinned_signals(bool notify)
{
    if (pinned_.empty())
        return;
    pinned_.clear();
    if (notify && on_pinned_changed_)
        on_pinned_changed_();
}

std::vector<std::string> timelinebar::get_pinned_vcd_names(void) const
{
    std::vector<std::string> out;
    out.reserve(pinned_.size());
    for (const auto &p : pinned_)
        out.push_back(p.ref.vcd_name);
    return out;
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
    /* Start and end times are always anchored to the full VCD; the bifocal lens handles magnification. */
    return {t_min, t_max};
}

double timelinebar::current_playhead_u(void) const
{
    if (!session_)
        return 0.0;
    const auto [t0, t1] = compute_time_window();
    const double cur_t = static_cast<double>(session_->get_current_timestamp());
    return std::clamp((cur_t - static_cast<double>(t0)) / static_cast<double>(std::max<uint64_t>(1, t1 - t0)),
                      0.0, 1.0);
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
        const int zb_size = 18;
        const int zb_y = y + (CLK_RULER_HEIGHT - zb_size) / 2;
        l.btn_zoom_in  = {wave_x - 6 - zb_size, zb_y, zb_size, zb_size};
        l.btn_zoom_out = {l.btn_zoom_in.x - 4 - zb_size, zb_y, zb_size, zb_size};
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

size_t timelinebar::tick_from_wave_x(int screen_x, const SDL_Rect &wave_area, const wave_lens &lens) const
{
    if (!session_ || session_->get_total_ticks() <= 1)
        return 0;
    const auto [t0, t1] = compute_time_window();
    const double rel = static_cast<double>(screen_x - wave_area.x) /
                       static_cast<double>(std::max(1, wave_area.w));
    const double u = lens.u_from_x(std::clamp(rel, 0.0, 1.0));
    const uint64_t target_t = t0 + static_cast<uint64_t>(std::llround(u * static_cast<double>(t1 - t0)));
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

    if (event.type == SDL_EVENT_MOUSE_WHEEL && has_active_wave()) {
        if (point_in_rect(hover_x_, hover_y_, l.wave_panel)) {
            if (event.wheel.y > 0.0f)
                zoom_in();
            else if (event.wheel.y < 0.0f)
                zoom_out();
            return true;
        }
    }

    if (event.type == SDL_EVENT_MOUSE_MOTION) {
        hover_x_ = static_cast<int>(event.motion.x);
        hover_y_ = static_cast<int>(event.motion.y);
        if (scrubbing_) {
            emit_seek(tick_from_x(hover_x_, bar_rect));
            return true;
        }
        if (scrubbing_wave_) {
            const wave_lens lens = build_lens(frozen_lens_center_u_);
            emit_seek(tick_from_wave_x(hover_x_, l.wave_area, lens));
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

        /* Mini zoom (-) and (+) buttons in the clk row signal column. */
        if (has_active_wave()) {
            if (point_in_rect(mx, my, l.btn_zoom_in)) {
                zoom_in();
                return true;
            }
            if (point_in_rect(mx, my, l.btn_zoom_out)) {
                zoom_out();
                return true;
            }
        }

        /* Check per-row unpin buttons first. */
        for (size_t i = 0; i < l.unpin_btns.size() && i < pinned_.size(); ++i) {
            if (point_in_rect(mx, my, l.unpin_btns[i])) {
                pinned_.erase(pinned_.begin() + static_cast<std::ptrdiff_t>(i));
                if (on_pinned_changed_)
                    on_pinned_changed_();
                return true;
            }
        }

        /* Clicking inside the waveform area freezes the lens center during drag and scrubs cleanly. */
        if (has_active_wave() && point_in_rect(mx, my, l.wave_area)) {
            frozen_lens_center_u_ = current_playhead_u();
            const wave_lens lens = build_lens(frozen_lens_center_u_);
            scrubbing_wave_ = true;
            emit_seek(tick_from_wave_x(mx, l.wave_area, lens));
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
        const size_t total_ticks = session_->get_total_ticks();

        /* Compute smoothly interpolated playhead time first so the bifocal lens glides at 60 FPS. */
        const size_t cur_idx = session_->get_current_tick();
        const uint64_t now_ms = SDL_GetTicks();
        const bool animating = clock_running && !scrubbing_ && !scrubbing_wave_ && !session_->is_unlocked();
        if (!animating || !was_playing_ || cur_idx != last_observed_tick_) {
            last_observed_tick_ = cur_idx;
            last_tick_ms_ = now_ms;
        }
        was_playing_ = animating;

        double playhead_t = static_cast<double>(session_->get_current_timestamp());
        if (animating && cur_idx + 1 < total_ticks) {
            constexpr double kTickIntervalMs = 500.0;
            const double frac = std::clamp(static_cast<double>(now_ms - last_tick_ms_) / kTickIntervalMs, 0.0, 1.0);
            const uint64_t next_t = session_->get_tick_timestamp(cur_idx + 1);
            if (next_t > session_->get_current_timestamp())
                playhead_t += frac * static_cast<double>(next_t - session_->get_current_timestamp());
        }

        const double playhead_u = std::clamp((playhead_t - static_cast<double>(t0)) * inv_span, 0.0, 1.0);
        const double lens_center_u = scrubbing_wave_ ? frozen_lens_center_u_ : playhead_u;
        const wave_lens lens = build_lens(lens_center_u);

        auto time_to_x = [&](double t) -> float {
            const double u = std::clamp((t - static_cast<double>(t0)) * inv_span, 0.0, 1.0);
            return static_cast<float>(l.wave_area.x) +
                   static_cast<float>(lens.x_from_u(u) * static_cast<double>(l.wave_area.w));
        };

        /* Deep GTKWave dark background + top divider border. */
        SDL_FRect panel_f = {static_cast<float>(l.wave_panel.x), static_cast<float>(l.wave_panel.y),
                             static_cast<float>(l.wave_panel.w), static_cast<float>(l.wave_panel.h)};
        SDL_SetRenderDrawColor(ren, 10, 12, 16, 245);
        SDL_RenderFillRect(ren, &panel_f);

        /* Subtle highlight band marking the uniform flat-top magnified region when lens is active. */
        if (lens.is_active()) {
            const double u_lo = std::clamp(lens.center - lens.half_flat, 0.0, 1.0);
            const double u_hi = std::clamp(lens.center + lens.half_flat, 0.0, 1.0);
            const float fx0 = static_cast<float>(l.wave_area.x) +
                              static_cast<float>(lens.x_from_u(u_lo) * static_cast<double>(l.wave_area.w));
            const float fx1 = static_cast<float>(l.wave_area.x) +
                              static_cast<float>(lens.x_from_u(u_hi) * static_cast<double>(l.wave_area.w));
            SDL_FRect flat_band = {fx0, static_cast<float>(l.wave_area.y),
                                   std::max(1.0f, fx1 - fx0), static_cast<float>(l.wave_area.h)};
            SDL_SetRenderDrawColor(ren, 255, 255, 255, 11);
            SDL_RenderFillRect(ren, &flat_band);
        }

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

        /* Adaptive screen-spaced clock-tick gridlines & labels (O(width/38 * log N) even on huge VCDs). */
        std::vector<std::pair<size_t, float>> labeled_ticks;
        if (total_ticks > 0 && l.wave_area.w > 0) {
            constexpr float kMinLabelGapPx = 38.0f;
            float last_gx = -1e9f;
            size_t last_tk = SIZE_MAX;
            for (int px_off = 0; px_off <= l.wave_area.w; px_off += 16) {
                const double sx = static_cast<double>(px_off) / static_cast<double>(std::max(1, l.wave_area.w));
                const double u = lens.u_from_x(sx);
                const uint64_t target_t = t0 + static_cast<uint64_t>(std::llround(u * static_cast<double>(t1 - t0)));
                const size_t tk = session_->find_nearest_tick(target_t);
                if (tk == last_tk)
                    continue;
                const float gx = time_to_x(static_cast<double>(session_->get_tick_timestamp(tk)));
                if (gx - last_gx < kMinLabelGapPx)
                    continue;
                last_tk = tk;
                last_gx = gx;
                labeled_ticks.emplace_back(tk, gx);
                SDL_SetRenderDrawColor(ren, 38, 46, 58, 170);
                SDL_RenderLine(ren, gx, static_cast<float>(l.wave_area.y),
                               gx, static_cast<float>(l.wave_area.y + l.wave_area.h));
            }
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

            draw_wave_trace(ren, canvas, row, e.data, &e.seg_labels, t0, t1, lens, false);

            SDL_SetRenderDrawColor(ren, 42, 48, 58, 200);
            SDL_RenderLine(ren, panel_f.x, static_cast<float>(row.y + row.h),
                           panel_f.x + panel_f.w, static_cast<float>(row.y + row.h));
        };

        /* Live hover row at the top (so pinned rows & clk ruler never shift vertically). */
        if (has_unpinned_hover() && l.hover_row.h > 0)
            draw_signal_row(hovered_, l.hover_row, nullptr);

        /* Unified clk wave + clock-tick numbers + mini (-) / (+) zoom buttons row. */
        if (l.clk_row.h > 0) {
            const int clk_lbl_w = std::max(1, l.btn_zoom_out.x - (l.wave_panel.x + 8) - 4);
            SDL_Rect clk_lbl = {l.wave_panel.x + 8, l.clk_row.y + 3, clk_lbl_w, CLK_RULER_HEIGHT - 6};
            const std::string clk_title = clk_wave_.signal_name.empty() ? "ticks" : clk_wave_.signal_name;
            draw_clipped_label(ren, canvas, clk_title, clk_lbl, false);

            auto draw_zoom_btn = [&](const SDL_Rect &r, std::string_view glyph, bool enabled) {
                const bool hov = enabled && point_in_rect(hover_x_, hover_y_, r);
                SDL_FRect rf = {static_cast<float>(r.x), static_cast<float>(r.y),
                                static_cast<float>(r.w), static_cast<float>(r.h)};
                SDL_SetRenderDrawColor(ren, hov ? 85 : (enabled ? 46 : 28),
                                       hov ? 105 : (enabled ? 54 : 32),
                                       hov ? 125 : (enabled ? 64 : 38), 230);
                SDL_RenderFillRect(ren, &rf);
                SDL_SetRenderDrawColor(ren, enabled ? 110 : 50, enabled ? 125 : 58, enabled ? 140 : 66, 200);
                SDL_RenderRect(ren, &rf);
                draw_clipped_label(ren, canvas, glyph, r, true);
            };
            draw_zoom_btn(l.btn_zoom_out, "-", lens_ticks_ != 0);
            draw_zoom_btn(l.btn_zoom_in, "+", total_ticks > 2 && lens_ticks_ != kZoomSteps.front());

            /* Draw clock tick numbers first (tucked inside the wave height) so the green clk wave
             * renders cleanly on top without any dark box interrupting the signal line. */
            for (const auto &[tk, gx] : labeled_ticks) {
                const int lx = static_cast<int>(std::lround(gx)) + 3;
                const int avail = (l.wave_area.x + l.wave_area.w) - lx;
                if (avail < 12)
                    continue;
                const std::string tlbl = std::format("{}", tk);
                SDL_Rect rlbl = {lx, l.clk_row.y + 4, std::min(avail, 48), CLK_RULER_HEIGHT - 8};
                draw_clipped_label(ren, canvas, tlbl, rlbl, false);
            }

            if (!clk_wave_.segments.empty())
                draw_wave_trace(ren, canvas, l.clk_row, clk_wave_, nullptr, t0, t1, lens, true);

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
        const float px = time_to_x(playhead_t);
        SDL_SetRenderDrawColor(ren, 235, 245, 255, 230);
        SDL_RenderLine(ren, px, static_cast<float>(l.wave_area.y),
                       px, static_cast<float>(l.wave_area.y + l.wave_area.h));
        SDL_RenderLine(ren, px + 1.0f, static_cast<float>(l.wave_area.y),
                       px + 1.0f, static_cast<float>(l.wave_area.y + l.wave_area.h));
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
