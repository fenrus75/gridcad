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
#include <cmath>
#include <random>
#include <unordered_set>
#include <vector>

namespace {

constexpr int DIR_X[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
constexpr int DIR_Y[8] = {-1, -1, -1, 0, 0, 1, 1, 1};

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

/*
 * Fast geometric cost for a single wire segment between its two endpoint ports:
 *  - Base Manhattan wirelength (weighted by bus width; 3x for model_const)
 *  - Smooth straight-wire discount (up to 2x cheaper for pure horizontal/vertical,
 *    tapering smoothly with offset so elements aren't trapped on a 10x rail cliff)
 *  - Backward-edge / U-turn penalty when dx < 0 (right-to-left)
 */
double wire_cost(const wire *w)
{
    if (!w || is_global_control_wire(w))
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

/*
 * Fast AABB collision check with port-clearance padding (pad_x = 2, pad_y = 1
 * for non-connectors) so elements keep an open routing lane between ports.
 * Optionally ignores a second element (for swap validation) and reports a blocker.
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
        const int o_pad_x = other_conn ? 0 : 1;
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

/* Compute combined cost of all unique wires incident to two elements (for swap evaluation). */
double compute_pair_cost(element *a, element *b)
{
    double total = 0.0;
    std::unordered_set<const wire *> seen;
    for (element *e : {a, b}) {
        if (!e)
            continue;
        for (unsigned int pi = 0;; ++pi) {
            port *p = e->port_at(pi);
            if (!p)
                break;
            for (const auto &w : p->get_wires()) {
                if (!w || !seen.insert(w.get()).second)
                    continue;
                total += wire_cost(w.get());
            }
        }
    }
    return total;
}

/*
 * Try moving a single element using:
 *  1. 1-cell steps in all 8 directions (+ up to max_follow_steps in the best direction)
 *  2. Leap-over jumps of 3 or 5 cells in any of the 8 directions (to jump over jams)
 *  3. Swapping with an adjacent blocking non-pinned element if that strictly lowers cost
 * Returns the positive cost saved (0.0 if no improving move was found).
 */
double try_improve_element(scene *target_scene, element *elem, int max_follow_steps,
                           int &out_dx, int &out_dy, int &out_steps)
{
    out_dx = 0;
    out_dy = 0;
    out_steps = 0;

    const int orig_x = static_cast<int>(std::floor(elem->get_X()));
    const int orig_y = static_cast<int>(std::floor(elem->get_Y()));
    const double before_local = placement_optimizer::compute_element_cost(elem);

    int best_x = orig_x;
    int best_y = orig_y;
    int best_dir = -1;
    int best_stride = 1;
    double best_local = before_local;
    std::vector<element *> blockers;

    /* Evaluate 1-cell steps and jump-over strides (3, 5) in all 8 directions. */
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
            const double after_local = placement_optimizer::compute_element_cost(elem);
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

    /* Also check swapping with any adjacent blocking element to break traffic jams. */
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
        const double pair_before = compute_pair_cost(elem, other);
        elem->place(ox, oy);
        elem->reseat();
        other->place(orig_x, orig_y);
        other->reseat();
        const double pair_after = compute_pair_cost(elem, other);

        /* Revert for now. */
        elem->place(orig_x, orig_y);
        elem->reseat();
        other->place(ox, oy);
        other->reseat();

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
        out_dx = ox - orig_x;
        out_dy = oy - orig_y;
        out_steps = 1;
        return best_swap_saving;
    }

    if (best_dir >= 0) {
        const int dx = DIR_X[best_dir];
        const int dy = DIR_Y[best_dir];
        int cur_x = best_x;
        int cur_y = best_y;
        double cur_local = best_local;
        int steps = best_stride;

        elem->place(cur_x, cur_y);
        elem->reseat();

        /* Follow the winning direction up to max_follow_steps more unit steps while improving. */
        for (int f = 0; f < max_follow_steps; ++f) {
            const int nx = cur_x + dx;
            const int ny = cur_y + dy;
            if (!can_place_at(target_scene, elem, nx, ny))
                break;
            elem->place(nx, ny);
            elem->reseat();
            const double next_local = placement_optimizer::compute_element_cost(elem);
            if (next_local + 1e-6 < cur_local) {
                cur_x = nx;
                cur_y = ny;
                cur_local = next_local;
                steps++;
            } else {
                elem->place(cur_x, cur_y);
                elem->reseat();
                break;
            }
        }

        reseat_element_wires(elem, /*clear_routes=*/true);
        out_dx = dx;
        out_dy = dy;
        out_steps = steps;
        return before_local - cur_local;
    }

    return 0.0;
}

template <typename Pred>
placement_optimizer::step_result optimize_matching_elements(
    class scene *target_scene, Pred &&matches, std::string_view label, int max_follow_steps)
{
    placement_optimizer::step_result res;
    if (!target_scene || target_scene->elements.empty())
        return res;

    res.cost_before = placement_optimizer::compute_scene_cost(target_scene);
    double total_saved = 0.0;
    int moved_count = 0;

    for (auto &e : target_scene->elements) {
        element *elem = e.get();
        if (!matches(elem))
            continue;
        int dx = 0, dy = 0, steps = 0;
        const double saved = try_improve_element(target_scene, elem, max_follow_steps, dx, dy, steps);
        if (saved > 0.0) {
            total_saved += saved;
            moved_count++;
        }
    }

    res.moved = (moved_count > 0);
    res.steps_taken = moved_count;
    res.moved_element = std::string(label);
    res.cost_after = res.cost_before - total_saved;
    return res;
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

double placement_optimizer::compute_scene_cost(class scene *target_scene)
{
    if (!target_scene)
        return 0.0;

    double total = 0.0;
    std::unordered_set<const wire *> seen;
    for (const auto &elem : target_scene->elements) {
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
    }
    return total;
}

placement_optimizer::step_result placement_optimizer::optimize_one_round(
    class scene *target_scene, int max_follow_steps, int max_attempts)
{
    step_result res;
    if (!target_scene || target_scene->elements.empty())
        return res;

    std::vector<element *> movable;
    movable.reserve(target_scene->elements.size());
    for (auto &elem : target_scene->elements) {
        if (!elem->is_background() && !is_pinned_element(elem.get()))
            movable.push_back(elem.get());
    }
    if (movable.empty())
        return res;

    res.cost_before = compute_scene_cost(target_scene);
    res.cost_after = res.cost_before;

    static std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<size_t> elem_dist(0, movable.size() - 1);

    for (int attempt = 0; attempt < max_attempts; ++attempt) {
        element *elem = movable[elem_dist(rng)];
        int dx = 0, dy = 0, steps = 0;
        const double saved = try_improve_element(target_scene, elem, max_follow_steps, dx, dy, steps);
        if (saved > 0.0) {
            res.moved = true;
            res.steps_taken = steps;
            res.moved_element = elem->get_name().empty() ? elem->class_id() : elem->get_name();
            res.dx = dx;
            res.dy = dy;
            res.cost_after = res.cost_before - saved;
            return res;
        }
    }

    return res;
}

placement_optimizer::step_result placement_optimizer::optimize_all_connectors(
    class scene *target_scene, int max_follow_steps)
{
    return optimize_matching_elements(
        target_scene,
        [](element *e) { return e->class_id() == "connector:"; },
        "connectors",
        max_follow_steps);
}

placement_optimizer::step_result placement_optimizer::optimize_all_constants(
    class scene *target_scene, int max_follow_steps)
{
    return optimize_matching_elements(
        target_scene,
        [](element *e) { return e->class_id() == "model_const:"; },
        "constants",
        max_follow_steps);
}

placement_optimizer::step_result placement_optimizer::optimize_all_slices_and_concats(
    class scene *target_scene, int max_follow_steps)
{
    return optimize_matching_elements(
        target_scene,
        [](element *e) {
            const std::string cid = e->class_id();
            return cid == "model_slice:" || cid == "model_concat:";
        },
        "slices/concats",
        max_follow_steps);
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
