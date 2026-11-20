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
 *  - Straight-wire bonus (0 bend penalty for pure horizontal/vertical wires,
 *    +bend_penalty when dx != 0 && dy != 0)
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

    double cost = weight * (adx + ady);

    /* Pure horizontal (dy == 0, dx > 0) or pure vertical (dx == 0) gets 0 bend penalty. */
    const bool pure_horiz = (ady < 0.01 && dx >= 0.0);
    const bool pure_vert = (adx < 0.01);
    if (!pure_horiz && !pure_vert)
        cost += 4.0 * weight;

    /* Backward-edge penalty (lighter for sequential/submodule feedback loops). */
    if (dx < 0.0) {
        const bool seq = (src->parent && is_sequential(src->parent));
        const double back_mult = seq ? 0.4 : 1.5;
        cost += weight * (back_mult * adx + 4.0);
    }

    return cost;
}

/* Fast AABB collision check including 1-cell port margin around every element. */
bool can_place_at(const scene *s, const element *myself, int new_x, int new_y)
{
    const int mw = std::max(1, static_cast<int>(std::ceil(const_cast<element *>(myself)->get_width())));
    const int mh = std::max(1, static_cast<int>(std::ceil(const_cast<element *>(myself)->get_height())));

    if (new_x < 2 || new_y < 2 ||
        new_x + mw + 2 > static_cast<int>(s->sizeX) ||
        new_y + mh + 2 > static_cast<int>(s->sizeY)) {
        return false;
    }

    const bool self_conn = (const_cast<element *>(myself)->class_id() == "connector:");
    const int m_pad = self_conn ? 0 : 1;
    const int ax1 = new_x - m_pad;
    const int ay1 = new_y - m_pad;
    const int ax2 = new_x + mw + m_pad;
    const int ay2 = new_y + mh + m_pad;

    for (const auto &other : s->elements) {
        if (other.get() == myself || other->is_background())
            continue;
        const int ox = static_cast<int>(std::floor(other->get_X()));
        const int oy = static_cast<int>(std::floor(other->get_Y()));
        const int ow = std::max(1, static_cast<int>(std::ceil(other->get_width())));
        const int oh = std::max(1, static_cast<int>(std::ceil(other->get_height())));
        const bool other_conn = (other->class_id() == "connector:");
        const int o_pad = other_conn ? 0 : 1;

        const int bx1 = ox - o_pad;
        const int by1 = oy - o_pad;
        const int bx2 = ox + ow + o_pad;
        const int by2 = oy + oh + o_pad;

        if (ax1 < bx2 && ax2 > bx1 && ay1 < by2 && ay2 > by1)
            return false;
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
        const int orig_x = static_cast<int>(std::floor(elem->get_X()));
        const int orig_y = static_cast<int>(std::floor(elem->get_Y()));
        const double before_local = compute_element_cost(elem);

        /* Evaluate all 8 directions and pick the steepest strictly-improving move. */
        int best_dir = -1;
        double best_local = before_local;

        for (int dir = 0; dir < 8; ++dir) {
            const int cand_x = orig_x + DIR_X[dir];
            const int cand_y = orig_y + DIR_Y[dir];
            if (!can_place_at(target_scene, elem, cand_x, cand_y))
                continue;

            elem->place(cand_x, cand_y);
            elem->reseat();
            const double after_local = compute_element_cost(elem);
            if (after_local + 1e-6 < best_local) {
                best_local = after_local;
                best_dir = dir;
            }
        }

        if (best_dir >= 0) {
            const int dx = DIR_X[best_dir];
            const int dy = DIR_Y[best_dir];
            int cur_x = orig_x + dx;
            int cur_y = orig_y + dy;
            double cur_local = best_local;
            int steps = 1;

            elem->place(cur_x, cur_y);
            elem->reseat();

            /* Follow the winning direction up to max_follow_steps more times while improving. */
            for (int f = 0; f < max_follow_steps; ++f) {
                const int nx = cur_x + dx;
                const int ny = cur_y + dy;
                if (!can_place_at(target_scene, elem, nx, ny))
                    break;
                elem->place(nx, ny);
                elem->reseat();
                const double next_local = compute_element_cost(elem);
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

            res.moved = true;
            res.steps_taken = steps;
            res.moved_element = elem->get_name().empty() ? elem->class_id() : elem->get_name();
            res.dx = dx;
            res.dy = dy;
            res.cost_after = res.cost_before - (before_local - cur_local);
            return res;
        }

        /* Restore original position before trying another element. */
        elem->place(orig_x, orig_y);
        elem->reseat();
    }

    return res;
}

placement_optimizer::step_result placement_optimizer::optimize_all_connectors(
    class scene *target_scene, int max_follow_steps)
{
    step_result res;
    if (!target_scene || target_scene->elements.empty())
        return res;

    res.cost_before = compute_scene_cost(target_scene);
    double total_saved = 0.0;
    int moved_connectors = 0;

    for (auto &e : target_scene->elements) {
        element *elem = e.get();
        if (elem->class_id() != "connector:")
            continue;

        const int orig_x = static_cast<int>(std::floor(elem->get_X()));
        const int orig_y = static_cast<int>(std::floor(elem->get_Y()));
        const double before_local = compute_element_cost(elem);

        int best_dir = -1;
        double best_local = before_local;
        for (int dir = 0; dir < 8; ++dir) {
            const int cand_x = orig_x + DIR_X[dir];
            const int cand_y = orig_y + DIR_Y[dir];
            if (!can_place_at(target_scene, elem, cand_x, cand_y))
                continue;
            elem->place(cand_x, cand_y);
            elem->reseat();
            const double after_local = compute_element_cost(elem);
            if (after_local + 1e-6 < best_local) {
                best_local = after_local;
                best_dir = dir;
            }
        }

        if (best_dir >= 0) {
            const int dx = DIR_X[best_dir];
            const int dy = DIR_Y[best_dir];
            int cur_x = orig_x + dx;
            int cur_y = orig_y + dy;
            double cur_local = best_local;

            elem->place(cur_x, cur_y);
            elem->reseat();

            for (int f = 0; f < max_follow_steps; ++f) {
                const int nx = cur_x + dx;
                const int ny = cur_y + dy;
                if (!can_place_at(target_scene, elem, nx, ny))
                    break;
                elem->place(nx, ny);
                elem->reseat();
                const double next_local = compute_element_cost(elem);
                if (next_local + 1e-6 < cur_local) {
                    cur_x = nx;
                    cur_y = ny;
                    cur_local = next_local;
                } else {
                    elem->place(cur_x, cur_y);
                    elem->reseat();
                    break;
                }
            }

            reseat_element_wires(elem, /*clear_routes=*/true);
            total_saved += (before_local - cur_local);
            moved_connectors++;
        } else {
            elem->place(orig_x, orig_y);
            elem->reseat();
        }
    }

    res.moved = (moved_connectors > 0);
    res.steps_taken = moved_connectors;
    res.moved_element = "connectors";
    res.cost_after = res.cost_before - total_saved;
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
