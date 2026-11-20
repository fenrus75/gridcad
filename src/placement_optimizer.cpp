/*
 * gridcad -- a tool to do digital circuit design with a GUI
 *
 * (C) Copyright 2025  -- Arjan van de Ven
 *
 * Licensed under the terms of the
 * GNU General Public LIcense version 3.0
 */

#include "placement_optimizer.h"
#include "gridcad.h"
#include "scene.h"
#include "element.h"
#include "port.h"
#include "wire.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {

constexpr int DIR_X[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
constexpr int DIR_Y[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
constexpr double CROSS_PENALTY = 8.0;
constexpr double SIBLING_DIST_WEIGHT = 0.75;
constexpr double SIBLING_ALIGN_PENALTY = 6.0;
constexpr double REPULSION_STRENGTH = 140.0;
constexpr double REPULSION_RADIUS2 = 28.0 * 28.0;
constexpr int GRID_SNAP_STEP = 16;
constexpr double GRID_SNAP_DIST_WEIGHT = 0.15;
constexpr double GRID_SNAP_PENALTY = 2.5;

bool is_pinned_element(const element *elem)
{
    const std::string cid = const_cast<element *>(elem)->class_id();
    /* Pin primary I/O ports; leave model_const and all internal logic/submodules free to move. */
    return cid == "model_toggle:" || cid == "model_clock:" || cid == "model_output:";
}

bool is_constant(const element *elem)
{
    return const_cast<element *>(elem)->class_id() == "model_const:";
}

bool is_sequential(const element *elem)
{
    const std::string cid = const_cast<element *>(elem)->class_id();
    return cid == "model_reg:" || cid == "model_dflipflop:" ||
           cid == "model_deflipflop:" || cid == "model_nest:";
}

bool is_global_control_wire(const wire *w)
{
    if (!w)
        return true;
    if (w->value.is_clock)
        return true;
    if (w->name == "clk" || w->name == "rst" || w->name == "rst_n")
        return true;
    return false;
}

/* Split a name like "gridcad_lut_4" into ("gridcad_lut_", 4). Returns false if no trailing digits. */
bool split_name_prefix_number(std::string_view name, std::string &out_prefix, int &out_num)
{
    if (name.empty())
        return false;
    size_t pos = name.size();
    while (pos > 0 && std::isdigit(static_cast<unsigned char>(name[pos - 1])))
        --pos;
    if (pos == name.size() || pos == 0)
        return false;

    int parsed = 0;
    auto [ptr, ec] = std::from_chars(name.data() + pos, name.data() + name.size(), parsed);
    if (ec != std::errc{} || ptr != name.data() + name.size())
        return false;

    out_prefix.assign(name.substr(0, pos));
    out_num = parsed;
    return true;
}

/*
 * Fast geometric cost for a single wire segment between its two endpoint ports:
 *  - Base Manhattan wirelength (weighted by bus width; 3x for model_const)
 *  - Smooth straight-wire discount (up to 2x cheaper for pure horizontal/vertical)
 *  - Backward-edge / U-turn penalty when dx < 0 (right-to-left)
 */
double wire_cost(const wire *w, bool include_global = false)
{
    if (!w || (!include_global && is_global_control_wire(w)))
        return 0.0;

    const auto &pts = w->get_ports();
    if (pts.size() < 2 || !pts[0] || !pts[1])
        return 0.0;

    const port *src = pts[0];
    const port *dst = pts[1];
    if (src->direction == PORT_IN && dst->direction != PORT_IN)
        std::swap(src, dst);

    const double dx = static_cast<double>(dst->screenX - src->screenX);
    const double dy = static_cast<double>(dst->screenY - src->screenY);
    const double adx = std::abs(dx);
    const double ady = std::abs(dy);

    double weight = (const_cast<wire *>(w)->get_width() > 1) ? 1.5 : 1.0;
    if ((src->parent && is_constant(src->parent)) || (dst->parent && is_constant(dst->parent)))
        weight *= 3.0;

    /* Smooth alignment valley: 0.5x at exact alignment, smoothly approaching 1.0x. */
    const double horiz_factor = (dx >= 0.0) ? (1.0 - 0.5 / (1.0 + ady)) : 1.0;
    const double vert_factor = 1.0 - 0.5 / (1.0 + adx);
    const double align_factor = std::min(horiz_factor, vert_factor);

    double cost = weight * (adx + ady) * align_factor;

    if (adx >= 0.01 && ady >= 0.01)
        cost += 1.5 * weight;

    /* Backward-edge penalty (lighter for sequential/submodule feedback loops). */
    if (dx < 0.0) {
        const bool seq = (src->parent && is_sequential(src->parent));
        const double back_mult = seq ? 0.4 : 1.5;
        cost += weight * (back_mult * adx + 4.0);
    }

    return cost;
}

/* Flat, cache-friendly wire segment representation for fast incremental crossing counts. */
struct wire_seg {
    float x1 = 0.0f, y1 = 0.0f;
    float x2 = 0.0f, y2 = 0.0f;
    const port *p1 = nullptr;
    const port *p2 = nullptr;
    const wire *w = nullptr;
};

void refresh_seg(wire_seg &s)
{
    if (s.p1 && s.p2) {
        s.x1 = s.p1->screenX;
        s.y1 = s.p1->screenY;
        s.x2 = s.p2->screenX;
        s.y2 = s.p2->screenY;
    }
}

/* Fast AABB-rejected 2D segment intersection test (strict interior crossing; ignores shared endpoints). */
inline bool segments_cross(const wire_seg &a, const wire_seg &b)
{
    if (a.p1 == b.p1 || a.p1 == b.p2 || a.p2 == b.p1 || a.p2 == b.p2)
        return false;

    const float ax_min = std::min(a.x1, a.x2), ax_max = std::max(a.x1, a.x2);
    const float bx_min = std::min(b.x1, b.x2), bx_max = std::max(b.x1, b.x2);
    if (ax_max <= bx_min || bx_max <= ax_min)
        return false;

    const float ay_min = std::min(a.y1, a.y2), ay_max = std::max(a.y1, a.y2);
    const float by_min = std::min(b.y1, b.y2), by_max = std::max(b.y1, b.y2);
    if (ay_max <= by_min || by_max <= ay_min)
        return false;

    const float abx = a.x2 - a.x1, aby = a.y2 - a.y1;
    const float o1 = abx * (b.y1 - a.y1) - aby * (b.x1 - a.x1);
    const float o2 = abx * (b.y2 - a.y1) - aby * (b.x2 - a.x1);
    if ((o1 >= 0.0f && o2 >= 0.0f) || (o1 <= 0.0f && o2 <= 0.0f))
        return false;

    const float cdx = b.x2 - b.x1, cdy = b.y2 - b.y1;
    const float o3 = cdx * (a.y1 - b.y1) - cdy * (a.x1 - b.x1);
    const float o4 = cdx * (a.y2 - b.y1) - cdy * (a.x2 - b.x1);
    if ((o3 >= 0.0f && o4 >= 0.0f) || (o3 <= 0.0f && o4 <= 0.0f))
        return false;

    return true;
}

/*
 * Consecutive-numbered sibling alignment cost:
 * Rewards placing elem_b (e.g. "gridcad_lut_1") directly below elem_a ("gridcad_lut_0")
 * at the same X with 1 free space between their port footprints (y_b == y_a + h_a + 2).
 */
double sibling_pair_cost(const element *a, const element *b)
{
    if (!a || !b)
        return 0.0;
    const double ax = std::floor(const_cast<element *>(a)->get_X());
    const double ay = std::floor(const_cast<element *>(a)->get_Y());
    const double ah = std::max(1.0, std::ceil(static_cast<double>(const_cast<element *>(a)->get_height())));
    const double bx = std::floor(const_cast<element *>(b)->get_X());
    const double by = std::floor(const_cast<element *>(b)->get_Y());

    const double target_y = ay + ah + 2.0;
    const double dx = std::abs(bx - ax);
    const double dy = std::abs(by - target_y);

    double cost = SIBLING_DIST_WEIGHT * (dx + dy);
    if (dx > 0.01 || dy > 0.01)
        cost += SIBLING_ALIGN_PENALTY;
    return cost;
}

bool is_repelling_block(const element *elem)
{
    if (!elem || const_cast<element *>(elem)->is_background())
        return false;
    const std::string cid = const_cast<element *>(elem)->class_id();
    /* Connectors and constants are tiny helpers that should hug their neighbors, not repel. */
    return cid != "connector:" && cid != "model_const:";
}

/* Gentle grid-snap cost: rewards X or Y being a multiple of 16 (double bonus for both). */
double grid_snap_cost(const element *e)
{
    if (!e || !is_repelling_block(e) || is_pinned_element(e))
        return 0.0;
    const int x = static_cast<int>(std::floor(const_cast<element *>(e)->get_X()));
    const int y = static_cast<int>(std::floor(const_cast<element *>(e)->get_Y()));
    const int rx = ((x % GRID_SNAP_STEP) + GRID_SNAP_STEP) % GRID_SNAP_STEP;
    const int ry = ((y % GRID_SNAP_STEP) + GRID_SNAP_STEP) % GRID_SNAP_STEP;
    const int dx = std::min(rx, GRID_SNAP_STEP - rx);
    const int dy = std::min(ry, GRID_SNAP_STEP - ry);

    double c = GRID_SNAP_DIST_WEIGHT * static_cast<double>(dx + dy);
    if (dx != 0)
        c += GRID_SNAP_PENALTY;
    if (dy != 0)
        c += GRID_SNAP_PENALTY;
    return c;
}

/* Inverse-square Coulomb repulsion energy between two unrelated blocks within cutoff radius. */
double repulsion_pair_cost(const element *a, const element *b)
{
    const double ax = const_cast<element *>(a)->get_X() + 0.5 * const_cast<element *>(a)->get_width();
    const double ay = const_cast<element *>(a)->get_Y() + 0.5 * const_cast<element *>(a)->get_height();
    const double bx = const_cast<element *>(b)->get_X() + 0.5 * const_cast<element *>(b)->get_width();
    const double by = const_cast<element *>(b)->get_Y() + 0.5 * const_cast<element *>(b)->get_height();
    const double dx = ax - bx;
    const double dy = ay - by;
    const double d2 = dx * dx + dy * dy;
    if (d2 >= REPULSION_RADIUS2)
        return 0.0;
    return REPULSION_STRENGTH / std::max(4.0, d2);
}

/* Precomputed scene-wide index for O(degree * W) incremental crossing, sibling & repulsion evaluation. */
struct opt_context {
    std::vector<wire_seg> segs;
    std::unordered_map<const element *, std::vector<size_t>> elem_segs;
    std::unordered_map<const element *, const element *> prev_sibling;
    std::unordered_map<const element *, const element *> next_sibling;
    std::vector<const element *> repelling_blocks;
    std::unordered_map<const element *, std::unordered_set<const element *>> connected_neighbors;

    explicit opt_context(scene *s)
    {
        if (!s)
            return;

        /* 1. Build flat list of non-clock wire segments and element -> segment index map. */
        std::unordered_map<const wire *, size_t> wire_to_idx;
        for (const auto &elem : s->elements) {
            if (is_repelling_block(elem.get()))
                repelling_blocks.push_back(elem.get());

            for (unsigned int pi = 0;; ++pi) {
                port *p = elem->port_at(pi);
                if (!p)
                    break;
                for (const auto &w : p->get_wires()) {
                    if (!w || is_global_control_wire(w.get()))
                        continue;
                    auto it = wire_to_idx.find(w.get());
                    if (it == wire_to_idx.end()) {
                        const auto &pts = w->get_ports();
                        if (pts.size() < 2 || !pts[0] || !pts[1])
                            continue;
                        wire_seg seg;
                        seg.p1 = pts[0];
                        seg.p2 = pts[1];
                        seg.w = w.get();
                        refresh_seg(seg);
                        const size_t idx = segs.size();
                        segs.push_back(seg);
                        wire_to_idx[w.get()] = idx;
                        if (seg.p1->parent)
                            elem_segs[seg.p1->parent].push_back(idx);
                        if (seg.p2->parent && seg.p2->parent != seg.p1->parent) {
                            elem_segs[seg.p2->parent].push_back(idx);
                            if (seg.p1->parent) {
                                connected_neighbors[seg.p1->parent].insert(seg.p2->parent);
                                connected_neighbors[seg.p2->parent].insert(seg.p1->parent);
                            }
                        }
                    }
                }
            }
        }

        /* 2. Link consecutive-numbered same-prefix siblings (e.g. gridcad_lut_0 -> gridcad_lut_1). */
        std::unordered_map<std::string, std::unordered_map<int, const element *>> groups;
        for (const auto &elem : s->elements) {
            if (elem->is_background() || is_pinned_element(elem.get()))
                continue;
            std::string prefix;
            int num = 0;
            if (split_name_prefix_number(elem->get_name(), prefix, num))
                groups[prefix][num] = elem.get();
        }
        for (const auto &[prefix, by_num] : groups) {
            for (const auto &[num, elem] : by_num) {
                auto nit = by_num.find(num + 1);
                if (nit != by_num.end()) {
                    next_sibling[elem] = nit->second;
                    prev_sibling[nit->second] = elem;
                    /* Siblings attract rather than repel each other. */
                    connected_neighbors[elem].insert(nit->second);
                    connected_neighbors[nit->second].insert(elem);
                }
            }
        }
    }

    bool are_related(const element *a, const element *b) const
    {
        auto it = connected_neighbors.find(a);
        return it != connected_neighbors.end() && it->second.count(b) > 0;
    }

    double repulsion_cost_for(const element *e) const
    {
        if (!is_repelling_block(e))
            return 0.0;
        double c = 0.0;
        for (const element *other : repelling_blocks) {
            if (other == e || are_related(e, other))
                continue;
            c += repulsion_pair_cost(e, other);
        }
        return c;
    }

    double all_repulsion_cost(void) const
    {
        double c = 0.0;
        const size_t n = repelling_blocks.size();
        for (size_t i = 0; i < n; ++i) {
            for (size_t j = i + 1; j < n; ++j) {
                if (!are_related(repelling_blocks[i], repelling_blocks[j]))
                    c += repulsion_pair_cost(repelling_blocks[i], repelling_blocks[j]);
            }
        }
        return c;
    }

    double all_grid_snap_cost(void) const
    {
        double c = 0.0;
        for (const element *e : repelling_blocks)
            c += grid_snap_cost(e);
        return c;
    }

    void refresh_element(const element *e)
    {
        auto it = elem_segs.find(e);
        if (it == elem_segs.end())
            return;
        for (size_t idx : it->second)
            refresh_seg(segs[idx]);
    }

    /* Count crossings involving any segment in `indices` against all other segments. */
    int count_crossings_for(const std::vector<size_t> &indices) const
    {
        int crossings = 0;
        const size_t total = segs.size();
        for (size_t k = 0; k < indices.size(); ++k) {
            const size_t i = indices[k];
            const wire_seg &sa = segs[i];
            for (size_t j = 0; j < total; ++j) {
                if (j == i)
                    continue;
                /* Avoid double-counting pairs where both segments are in `indices`. */
                bool j_in_set = false;
                for (size_t m = 0; m < indices.size(); ++m) {
                    if (indices[m] == j) {
                        j_in_set = true;
                        break;
                    }
                }
                if (j_in_set && j < i)
                    continue;
                if (segments_cross(sa, segs[j]))
                    crossings++;
            }
        }
        return crossings;
    }

    int count_all_crossings(void) const
    {
        int crossings = 0;
        const size_t total = segs.size();
        for (size_t i = 0; i < total; ++i) {
            for (size_t j = i + 1; j < total; ++j) {
                if (segments_cross(segs[i], segs[j]))
                    crossings++;
            }
        }
        return crossings;
    }

    double sibling_cost_for(const element *e) const
    {
        double c = 0.0;
        auto pit = prev_sibling.find(e);
        if (pit != prev_sibling.end())
            c += sibling_pair_cost(pit->second, e);
        auto nit = next_sibling.find(e);
        if (nit != next_sibling.end())
            c += sibling_pair_cost(e, nit->second);
        return c;
    }

    double all_sibling_cost(void) const
    {
        double c = 0.0;
        for (const auto &[a, b] : next_sibling)
            c += sibling_pair_cost(a, b);
        return c;
    }

    double local_cost(element *e) const
    {
        double total = sibling_cost_for(e) + repulsion_cost_for(e) + grid_snap_cost(e);
        auto it = elem_segs.find(e);
        if (it != elem_segs.end()) {
            for (size_t idx : it->second)
                total += wire_cost(segs[idx].w);
            total += CROSS_PENALTY * static_cast<double>(count_crossings_for(it->second));
        }
        /* Allow clk/rst trunk connectors to optimize their own wire length without pulling logic blocks. */
        if (e->class_id() == "connector:" && e->port_at(0)) {
            for (const auto &w : e->port_at(0)->get_wires()) {
                if (w && is_global_control_wire(w.get()))
                    total += wire_cost(w.get(), /*include_global=*/true);
            }
        }
        return total;
    }

    double pair_local_cost(element *a, element *b) const
    {
        std::vector<size_t> merged;
        auto ia = elem_segs.find(a);
        if (ia != elem_segs.end())
            merged.insert(merged.end(), ia->second.begin(), ia->second.end());
        auto ib = elem_segs.find(b);
        if (ib != elem_segs.end())
            merged.insert(merged.end(), ib->second.begin(), ib->second.end());
        std::sort(merged.begin(), merged.end());
        merged.erase(std::unique(merged.begin(), merged.end()), merged.end());

        double total = 0.0;
        for (size_t idx : merged)
            total += wire_cost(segs[idx].w);
        total += CROSS_PENALTY * static_cast<double>(count_crossings_for(merged));

        /* Sibling cost without double-counting if a and b are direct siblings. */
        total += sibling_cost_for(a) + sibling_cost_for(b);
        auto nit = next_sibling.find(a);
        if (nit != next_sibling.end() && nit->second == b)
            total -= sibling_pair_cost(a, b);
        auto pit = next_sibling.find(b);
        if (pit != next_sibling.end() && pit->second == a)
            total -= sibling_pair_cost(b, a);

        /* Repulsion cost without double-counting the (a, b) pair itself. */
        total += repulsion_cost_for(a) + repulsion_cost_for(b);
        if (is_repelling_block(a) && is_repelling_block(b) && !are_related(a, b))
            total -= repulsion_pair_cost(a, b);
        total += grid_snap_cost(a) + grid_snap_cost(b);
        return total;
    }
};

/*
 * Fast AABB collision check with port-clearance padding (pad_x = 2, pad_y = 1
 * for non-connectors) so elements keep an open routing lane between ports.
 */
bool can_place_at(const scene *s, const element *myself, int new_x, int new_y,
                  const element *ignore_other = nullptr, element **out_blocker = nullptr)
{
    if (out_blocker)
        *out_blocker = nullptr;

    const int mw = std::max(1, static_cast<int>(std::ceil(const_cast<element *>(myself)->get_width())));
    const int mh = std::max(1, static_cast<int>(std::ceil(const_cast<element *>(myself)->get_height())));

    if (new_x < 2 || new_y < 2 ||
        new_x + mw + 2 > static_cast<int>(s->sizeX) ||
        new_y + mh + 2 > static_cast<int>(s->sizeY)) {
        return false;
    }

    const bool self_conn = (const_cast<element *>(myself)->class_id() == "connector:");
    const int m_pad_x = self_conn ? 0 : 2;
    const int m_pad_y = self_conn ? 0 : 1;
    const int ax1 = new_x - m_pad_x;
    const int ay1 = new_y - m_pad_y;
    const int ax2 = new_x + mw + m_pad_x;
    const int ay2 = new_y + mh + m_pad_y;

    for (const auto &other : s->elements) {
        if (other.get() == myself || other.get() == ignore_other || other->is_background())
            continue;
        const int ox = static_cast<int>(std::floor(other->get_X()));
        const int oy = static_cast<int>(std::floor(other->get_Y()));
        const int ow = std::max(1, static_cast<int>(std::ceil(other->get_width())));
        const int oh = std::max(1, static_cast<int>(std::ceil(other->get_height())));
        const bool other_conn = (other->class_id() == "connector:");
        const int o_pad_x = other_conn ? 0 : (self_conn ? 2 : 1);
        const int o_pad_y = other_conn ? 0 : 1;

        const int bx1 = ox - o_pad_x;
        const int by1 = oy - o_pad_y;
        const int bx2 = ox + ow + o_pad_x;
        const int by2 = oy + oh + o_pad_y;

        if (ax1 < bx2 && ax2 > bx1 && ay1 < by2 && ay2 > by1) {
            if (out_blocker)
                *out_blocker = other.get();
            return false;
        }
    }
    return true;
}

void reseat_element_wires(element *elem, bool clear_routes)
{
    elem->reseat();
    for (unsigned int pi = 0;; ++pi) {
        port *p = elem->port_at(pi);
        if (!p)
            break;
        for (const auto &w : p->get_wires()) {
            if (!w)
                continue;
            w->reseat();
            if (clear_routes)
                w->clear_route();
        }
    }
}

/*
 * Try moving a single element using 8-direction steps, jump-over strides (3, 5),
 * adjacent-blocker swaps, and 1-in-100 opposite-direction logjam escape.
 */
double try_improve_element(scene *target_scene, opt_context &ctx, element *elem,
                           int max_follow_steps, int &out_dx, int &out_dy, int &out_steps)
{
    out_dx = 0;
    out_dy = 0;
    out_steps = 0;

    const int orig_x = static_cast<int>(std::floor(elem->get_X()));
    const int orig_y = static_cast<int>(std::floor(elem->get_Y()));
    const double before_local = ctx.local_cost(elem);

    int best_x = orig_x;
    int best_y = orig_y;
    int best_dir = -1;
    int best_stride = 1;
    double best_local = before_local;
    std::vector<element *> blockers;

    static constexpr int STRIDES[3] = {1, 3, 5};
    for (int dir = 0; dir < 8; ++dir) {
        for (int stride : STRIDES) {
            const int cand_x = orig_x + DIR_X[dir] * stride;
            const int cand_y = orig_y + DIR_Y[dir] * stride;
            element *blocker = nullptr;
            if (!can_place_at(target_scene, elem, cand_x, cand_y, nullptr, &blocker)) {
                if (stride == 1 && blocker && !is_pinned_element(blocker) && !blocker->is_background())
                    blockers.push_back(blocker);
                continue;
            }
            elem->place(cand_x, cand_y);
            elem->reseat();
            ctx.refresh_element(elem);
            const double after_local = ctx.local_cost(elem);
            if (after_local + 1e-6 < best_local) {
                best_local = after_local;
                best_x = cand_x;
                best_y = cand_y;
                best_dir = dir;
                best_stride = stride;
            }
        }
    }

    elem->place(orig_x, orig_y);
    elem->reseat();
    ctx.refresh_element(elem);

    /* Also check swapping with any adjacent blocking element to untangle crossings & jams. */
    element *best_swap = nullptr;
    double best_swap_saving = before_local - best_local;
    std::sort(blockers.begin(), blockers.end());
    blockers.erase(std::unique(blockers.begin(), blockers.end()), blockers.end());

    for (element *other : blockers) {
        const int ox = static_cast<int>(std::floor(other->get_X()));
        const int oy = static_cast<int>(std::floor(other->get_Y()));
        if (!can_place_at(target_scene, elem, ox, oy, other) ||
            !can_place_at(target_scene, other, orig_x, orig_y, elem)) {
            continue;
        }
        const double pair_before = ctx.pair_local_cost(elem, other);
        elem->place(ox, oy);
        elem->reseat();
        other->place(orig_x, orig_y);
        other->reseat();
        ctx.refresh_element(elem);
        ctx.refresh_element(other);
        const double pair_after = ctx.pair_local_cost(elem, other);

        elem->place(orig_x, orig_y);
        elem->reseat();
        other->place(ox, oy);
        other->reseat();
        ctx.refresh_element(elem);
        ctx.refresh_element(other);

        const double saving = pair_before - pair_after;
        if (saving > best_swap_saving + 1e-6) {
            best_swap_saving = saving;
            best_swap = other;
        }
    }

    if (best_swap) {
        const int ox = static_cast<int>(std::floor(best_swap->get_X()));
        const int oy = static_cast<int>(std::floor(best_swap->get_Y()));
        elem->place(ox, oy);
        best_swap->place(orig_x, orig_y);
        reseat_element_wires(elem, /*clear_routes=*/true);
        reseat_element_wires(best_swap, /*clear_routes=*/true);
        ctx.refresh_element(elem);
        ctx.refresh_element(best_swap);
        out_dx = ox - orig_x;
        out_dy = oy - orig_y;
        out_steps = 1;
        return best_swap_saving;
    }

    if (best_dir >= 0) {
        const int dx = DIR_X[best_dir];
        const int dy = DIR_Y[best_dir];

        /* 1-in-100 opposite-direction perturbation to escape local logjams. */
        static std::mt19937 perturb_rng{std::random_device{}()};
        if ((perturb_rng() % 100) == 0) {
            const int ox = orig_x - dx;
            const int oy = orig_y - dy;
            if (can_place_at(target_scene, elem, ox, oy)) {
                elem->place(ox, oy);
                reseat_element_wires(elem, /*clear_routes=*/true);
                ctx.refresh_element(elem);
                out_dx = -dx;
                out_dy = -dy;
                out_steps = 1;
                return 1e-6;
            }
        }

        int cur_x = best_x;
        int cur_y = best_y;
        double cur_local = best_local;
        int steps = best_stride;

        elem->place(cur_x, cur_y);
        elem->reseat();
        ctx.refresh_element(elem);

        for (int f = 0; f < max_follow_steps; ++f) {
            const int nx = cur_x + dx;
            const int ny = cur_y + dy;
            if (!can_place_at(target_scene, elem, nx, ny))
                break;
            elem->place(nx, ny);
            elem->reseat();
            ctx.refresh_element(elem);
            const double next_local = ctx.local_cost(elem);
            if (next_local + 1e-6 < cur_local) {
                cur_x = nx;
                cur_y = ny;
                cur_local = next_local;
                steps++;
            } else {
                elem->place(cur_x, cur_y);
                elem->reseat();
                ctx.refresh_element(elem);
                break;
            }
        }

        reseat_element_wires(elem, /*clear_routes=*/true);
        ctx.refresh_element(elem);
        out_dx = dx;
        out_dy = dy;
        out_steps = steps;
        return before_local - cur_local;
    }

    return 0.0;
}

} // namespace

double placement_optimizer::compute_element_cost(class element *elem)
{
    if (!elem)
        return 0.0;

    double total = 0.0;
    std::unordered_set<const wire *> seen;
    for (unsigned int pi = 0;; ++pi) {
        port *p = elem->port_at(pi);
        if (!p)
            break;
        for (const auto &w : p->get_wires()) {
            if (!w || !seen.insert(w.get()).second)
                continue;
            total += wire_cost(w.get());
        }
    }
    return total;
}

int placement_optimizer::count_scene_crossings(class scene *target_scene)
{
    if (!target_scene)
        return 0;
    opt_context ctx(target_scene);
    return ctx.count_all_crossings();
}

double placement_optimizer::compute_scene_cost(class scene *target_scene)
{
    if (!target_scene)
        return 0.0;

    opt_context ctx(target_scene);
    double total = 0.0;
    for (const auto &seg : ctx.segs)
        total += wire_cost(seg.w);
    total += CROSS_PENALTY * static_cast<double>(ctx.count_all_crossings());
    total += ctx.all_sibling_cost();
    total += ctx.all_repulsion_cost();
    total += ctx.all_grid_snap_cost();
    return total;
}

placement_optimizer::step_result placement_optimizer::optimize_all_elements(
    class scene *target_scene, int max_follow_steps)
{
    return optimize_rounds(target_scene, 1, max_follow_steps);
}

placement_optimizer::step_result placement_optimizer::optimize_rounds(
    class scene *target_scene, int rounds, int max_follow_steps)
{
    step_result res;
    if (!target_scene || target_scene->elements.empty() || rounds <= 0)
        return res;

    std::vector<element *> movable;
    movable.reserve(target_scene->elements.size());
    for (auto &e : target_scene->elements) {
        if (!e->is_background() && !is_pinned_element(e.get()))
            movable.push_back(e.get());
    }
    if (movable.empty())
        return res;

    opt_context ctx(target_scene);
    res.crossings_before = ctx.count_all_crossings();
    res.cost_before = 0.0;
    for (const auto &seg : ctx.segs)
        res.cost_before += wire_cost(seg.w);
    res.cost_before += CROSS_PENALTY * static_cast<double>(res.crossings_before);
    res.cost_before += ctx.all_sibling_cost();
    res.cost_before += ctx.all_repulsion_cost();
    res.cost_before += ctx.all_grid_snap_cost();

    static std::mt19937 rng{std::random_device{}()};

    int total_moved = 0;
    for (int r = 0; r < rounds; ++r) {
        std::shuffle(movable.begin(), movable.end(), rng);
        int round_moved = 0;
        for (element *elem : movable) {
            int dx = 0, dy = 0, steps = 0;
            if (try_improve_element(target_scene, ctx, elem, max_follow_steps, dx, dy, steps) > 0.0)
                round_moved++;
        }
        total_moved += round_moved;
        if (round_moved == 0)
            break;
    }

    res.moved = (total_moved > 0);
    res.steps_taken = total_moved;
    res.moved_element = "all_elements";
    res.crossings_after = ctx.count_all_crossings();
    res.cost_after = 0.0;
    for (const auto &seg : ctx.segs)
        res.cost_after += wire_cost(seg.w);
    res.cost_after += CROSS_PENALTY * static_cast<double>(res.crossings_after);
    res.cost_after += ctx.all_sibling_cost();
    res.cost_after += ctx.all_repulsion_cost();
    res.cost_after += ctx.all_grid_snap_cost();
    return res;
}

static bool g_import_view_mode = false;

bool is_import_view_mode(void)
{
    return g_import_view_mode;
}

void set_import_view_mode(bool enabled)
{
    g_import_view_mode = enabled;
}
