/*
 * gridcad -- a tool to do digital circuit design with a GUI
 *
 * (C) Copyright 2025  -- Arjan van de Ven
 *
 * Licensed under the terms of the
 * GNU General Public LIcense version 3.0
 */

#include "placer.h"
#include "scene.h"
#include "element.h"
#include "connector.h"
#include "port.h"
#include "wire.h"

#include <algorithm>
#include <cmath>
#include <queue>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {

bool is_primary_input(const element *elem)
{
    const std::string cid = const_cast<element *>(elem)->class_id();
    return cid == "model_toggle:" || cid == "model_clock:";
}

bool is_constant(const element *elem)
{
    return const_cast<element *>(elem)->class_id() == "model_const:";
}

bool is_slice(const element *elem)
{
    return const_cast<element *>(elem)->class_id() == "model_slice:";
}

bool is_primary_output(const element *elem)
{
    return const_cast<element *>(elem)->class_id() == "model_output:";
}

bool is_connector(const element *elem)
{
    return const_cast<element *>(elem)->class_id() == "connector:";
}

bool is_clock_element(const element *elem)
{
    const std::string cid = const_cast<element *>(elem)->class_id();
    if (cid == "model_clock:")
        return true;
    if (cid == "model_toggle:") {
        const std::string nm = const_cast<element *>(elem)->get_name();
        return nm == "clk";
    }
    return false;
}

bool is_reset_element(const element *elem)
{
    const std::string cid = const_cast<element *>(elem)->class_id();
    if (cid == "model_toggle:") {
        const std::string nm = const_cast<element *>(elem)->get_name();
        return nm == "rst" || nm == "rst_n";
    }
    return false;
}

/* Global broadcast control signals (clk/rst) should not distort dataflow layering or Y-ordering. */
bool is_global_control_port(const port *p)
{
    if (!p)
        return false;
    if (p->value.is_clock)
        return true;
    if (p->name == "clk" || p->name == "rst" || p->name == "rst_n")
        return true;
    if (p->parent && (is_clock_element(p->parent) || is_reset_element(p->parent)))
        return true;
    return false;
}

/* Sequential elements (registers and clocked submodules) break feedback cycles. */
bool is_cycle_breaker(const element *elem)
{
    const std::string cid = const_cast<element *>(elem)->class_id();
    if (cid == "model_reg:" || cid == "model_dflipflop:" || cid == "model_deflipflop:")
        return true;
    if (cid == "model_nest:") {
        element *e = const_cast<element *>(elem);
        for (unsigned int pi = 0;; ++pi) {
            port *p = e->port_at(pi);
            if (!p)
                break;
            if (p->direction == PORT_IN && p->name == "clk")
                return true;
        }
    }
    return false;
}

/* Follow wires from an output port, traversing through connector split junctions. */
void collect_sink_elements(port *start_port,
                           const std::unordered_map<const element *, size_t> &elem_index,
                           std::vector<size_t> &out_sinks)
{
    std::queue<port *> q;
    std::unordered_set<const port *> visited;
    q.push(start_port);
    visited.insert(start_port);

    while (!q.empty()) {
        port *cur = q.front();
        q.pop();
        for (const auto &w : cur->get_wires()) {
            if (!w)
                continue;
            for (port *other : w->get_ports()) {
                if (!other || other == cur || visited.count(other) || !other->parent)
                    continue;
                visited.insert(other);
                if (other->is_connector || is_connector(other->parent)) {
                    q.push(other);
                } else if (other->direction == PORT_IN) {
                    auto it = elem_index.find(other->parent);
                    if (it != elem_index.end())
                        out_sinks.push_back(it->second);
                }
            }
        }
    }
}

} // namespace

void topological_placer::place(class scene *target_scene)
{
    place(target_scene, options{});
}

void topological_placer::place(class scene *target_scene, const options &opts)
{
    if (!target_scene || target_scene->elements.empty())
        return;

    const size_t n = target_scene->elements.size();
    std::unordered_map<const element *, size_t> elem_index;
    elem_index.reserve(n);
    for (size_t i = 0; i < n; ++i)
        elem_index[target_scene->elements[i].get()] = i;

    /*
     * Build directed connectivity graphs over non-connector elements, tracing
     * transparently through connector split junctions and ignoring global
     * clk/rst broadcast nets:
     * - all_out/all_in: dataflow edges (used for barycenter Y-ordering)
     * - dag_out: combinational dataflow edges with cycle-breaker outputs
     *   omitted so sequential loops don't collapse layering.
     */
    std::vector<std::vector<size_t>> all_out(n), all_in(n);
    std::vector<std::vector<size_t>> dag_out(n);

    for (size_t u = 0; u < n; ++u) {
        element *src_elem = target_scene->elements[u].get();
        if (is_connector(src_elem))
            continue;

        const bool src_breaks_cycle = is_cycle_breaker(src_elem);
        const bool src_is_const = is_constant(src_elem);

        for (unsigned int pi = 0;; ++pi) {
            port *p = src_elem->port_at(pi);
            if (!p)
                break;
            if (p->direction != PORT_OUT)
                continue;
            if (is_global_control_port(p))
                continue;

            std::vector<size_t> sinks;
            collect_sink_elements(p, elem_index, sinks);
            for (size_t v : sinks) {
                if (v == u)
                    continue;
                all_out[u].push_back(v);
                all_in[v].push_back(u);

                if (!src_breaks_cycle && !src_is_const &&
                    !is_primary_output(target_scene->elements[v].get())) {
                    dag_out[u].push_back(v);
                }
            }
        }
    }

    auto dedup = [](std::vector<size_t> &vec) {
        std::sort(vec.begin(), vec.end());
        vec.erase(std::unique(vec.begin(), vec.end()), vec.end());
    };
    std::vector<int> indegree(n, 0);
    for (size_t u = 0; u < n; ++u) {
        dedup(all_out[u]);
        dedup(all_in[u]);
        dedup(dag_out[u]);
        for (size_t v : dag_out[u])
            indegree[v]++;
    }

    /* Longest-path topological layering via Kahn's algorithm + cyclic fallback peel. */
    std::vector<int> layer(n, 0);
    std::vector<bool> processed(n, false);
    std::queue<size_t> q;
    size_t active_count = 0;
    size_t processed_count = 0;

    for (size_t i = 0; i < n; ++i) {
        element *e = target_scene->elements[i].get();
        if (is_connector(e)) {
            processed[i] = true;
            continue;
        }
        active_count++;
        if (is_primary_input(e)) {
            layer[i] = 0;
        } else if (!is_constant(e) && !is_primary_output(e)) {
            layer[i] = 1;
        }
        if (indegree[i] == 0)
            q.push(i);
    }

    auto drain_queue = [&]() {
        while (!q.empty()) {
            size_t u = q.front();
            q.pop();
            if (processed[u])
                continue;
            processed[u] = true;
            processed_count++;
            for (size_t v : dag_out[u]) {
                if (layer[v] < layer[u] + 1)
                    layer[v] = layer[u] + 1;
                if (--indegree[v] == 0)
                    q.push(v);
            }
        }
    };

    drain_queue();

    /* If any combinational/submodule cycle remained, greedily break the lowest-indegree node. */
    while (processed_count < active_count) {
        size_t best = n;
        int best_in = INT_MAX;
        for (size_t i = 0; i < n; ++i) {
            if (!processed[i] && indegree[i] < best_in) {
                best_in = indegree[i];
                best = i;
            }
        }
        if (best == n)
            break;
        indegree[best] = 0;
        q.push(best);
        drain_queue();
    }

    /* Place constants directly in the column preceding their earliest consumer. */
    for (size_t i = 0; i < n; ++i) {
        if (!is_constant(target_scene->elements[i].get()))
            continue;
        int min_sink_layer = 1;
        bool has_sink = false;
        for (size_t v : all_out[i]) {
            if (!has_sink || layer[v] < min_sink_layer) {
                min_sink_layer = layer[v];
                has_sink = true;
            }
        }
        layer[i] = std::max(0, min_sink_layer - 1);
    }

    /*
     * Pull model_slice blocks adjacent to their earliest consumer so bit-taps
     * sit right in front of the LUTs/gates that consume them instead of
     * piling up on the far right and boomerang-routing wires across the layout.
     */
    for (size_t i = 0; i < n; ++i) {
        if (!is_slice(target_scene->elements[i].get()))
            continue;
        int min_sink_layer = INT_MAX;
        for (size_t v : all_out[i]) {
            if (!is_primary_output(target_scene->elements[v].get()))
                min_sink_layer = std::min(min_sink_layer, layer[v]);
        }
        if (min_sink_layer < INT_MAX)
            layer[i] = std::max(1, min_sink_layer - 1);
    }

    /* Place primary outputs in the rightmost column. */
    int max_internal_layer = 0;
    for (size_t i = 0; i < n; ++i) {
        element *e = target_scene->elements[i].get();
        if (!is_connector(e) && !is_primary_output(e))
            max_internal_layer = std::max(max_internal_layer, layer[i]);
    }
    const int output_layer = max_internal_layer + 1;
    for (size_t i = 0; i < n; ++i) {
        if (is_primary_output(target_scene->elements[i].get()))
            layer[i] = output_layer;
    }

    int num_layers = output_layer + 1;
    std::vector<std::vector<size_t>> columns(static_cast<size_t>(num_layers));
    for (size_t i = 0; i < n; ++i) {
        if (!is_connector(target_scene->elements[i].get()))
            columns[static_cast<size_t>(layer[i])].push_back(i);
    }

    /*
     * Order Column 0 inputs so data inputs sit at the top, reset below them,
     * and clk at the very bottom (best routing geometry for clock trees).
     */
    if (!columns.empty()) {
        std::stable_sort(columns[0].begin(), columns[0].end(),
                         [&](size_t a, size_t b) {
                             auto rank = [&](size_t idx) -> int {
                                 const element *e = target_scene->elements[idx].get();
                                 if (is_clock_element(e))
                                     return 2;
                                 if (is_reset_element(e))
                                     return 1;
                                 return 0;
                             };
                             return rank(a) < rank(b);
                         });
    }

    /*
     * Two-pass barycenter ordering (forward then backward) to align connected
     * slices, LUTs, and submodules vertically and minimize wire crossings.
     */
    std::vector<float> y_est(n, 0.0f);
    for (size_t idx = 0; idx < columns[0].size(); ++idx)
        y_est[columns[0][idx]] = static_cast<float>(idx);

    for (int c = 1; c < num_layers; ++c) {
        auto &col = columns[static_cast<size_t>(c)];
        std::vector<std::pair<float, size_t>> keyed;
        keyed.reserve(col.size());

        for (size_t idx = 0; idx < col.size(); ++idx) {
            size_t u = col[idx];
            float sum = 0.0f;
            int cnt = 0;
            for (size_t pred : all_in[u]) {
                if (layer[pred] < c) {
                    sum += y_est[pred];
                    cnt++;
                }
            }
            float key = (cnt > 0) ? (sum / static_cast<float>(cnt)) : static_cast<float>(idx);
            keyed.emplace_back(key, u);
        }

        std::stable_sort(keyed.begin(), keyed.end(),
                         [](const auto &a, const auto &b) { return a.first < b.first; });

        for (size_t idx = 0; idx < keyed.size(); ++idx) {
            col[idx] = keyed[idx].second;
            y_est[col[idx]] = static_cast<float>(idx);
        }
    }

    /* Backward barycenter pass on internal columns so slices/gates align with their sinks. */
    for (int c = num_layers - 2; c >= 1; --c) {
        auto &col = columns[static_cast<size_t>(c)];
        std::vector<std::pair<float, size_t>> keyed;
        keyed.reserve(col.size());

        for (size_t idx = 0; idx < col.size(); ++idx) {
            size_t u = col[idx];
            float sum = 0.0f;
            int cnt = 0;
            for (size_t succ : all_out[u]) {
                if (layer[succ] > c) {
                    sum += y_est[succ];
                    cnt++;
                }
            }
            float key = (cnt > 0) ? (sum / static_cast<float>(cnt)) : y_est[u];
            keyed.emplace_back(key, u);
        }

        std::stable_sort(keyed.begin(), keyed.end(),
                         [](const auto &a, const auto &b) { return a.first < b.first; });

        for (size_t idx = 0; idx < keyed.size(); ++idx) {
            col[idx] = keyed[idx].second;
            y_est[col[idx]] = static_cast<float>(idx);
        }
    }

    /*
     * Compute column X coordinates and element Y coordinates.
     * Split tall columns (> max_elems_per_subcol) into multiple sub-columns and
     * stagger odd rows horizontally so large modules stay widescreen & fast to route.
     */
    int cur_x = opts.margin_x;
    int max_x = opts.margin_x;
    int max_y = opts.margin_y;
    const size_t max_per_sub = static_cast<size_t>(std::max(1, opts.max_elems_per_subcol));

    for (int c = 0; c < num_layers; ++c) {
        const auto &col = columns[static_cast<size_t>(c)];
        if (col.empty())
            continue;

        const size_t num_subcols = (col.size() + max_per_sub - 1) / max_per_sub;
        const size_t per_sub = (col.size() + num_subcols - 1) / num_subcols;

        for (size_t sc = 0; sc < num_subcols; ++sc) {
            const size_t begin = sc * per_sub;
            const size_t end = std::min(col.size(), begin + per_sub);
            if (begin >= end)
                break;

            int col_width = 1;
            int cur_y = opts.margin_y;
            const bool do_stagger = (opts.stagger_x > 0) && ((end - begin) >= 2) &&
                                    (c > 0) && (c < num_layers - 1);

            for (size_t idx = begin; idx < end; ++idx) {
                element *elem = target_scene->elements[col[idx]].get();
                int w = std::max(1, static_cast<int>(std::ceil(elem->get_width())));
                int h = std::max(1, static_cast<int>(std::ceil(elem->get_height())));
                const int x_off = (do_stagger && ((idx - begin) % 2 == 1)) ? opts.stagger_x : 0;
                col_width = std::max(col_width, w + x_off);

                elem->place(cur_x + x_off, cur_y);
                elem->reseat();

                cur_y += h + opts.row_gap;
            }

            max_y = std::max(max_y, cur_y);
            const int density_bonus = std::min(16, static_cast<int>((end - begin) / 2));
            cur_x += col_width + opts.col_gap + density_bonus;
            max_x = std::max(max_x, cur_x);
        }
    }

    /* Ensure the scene grid comfortably encloses all elements and their ports. */
    target_scene->sizeX = std::max(200.0f, static_cast<float>(max_x + opts.margin_x));
    target_scene->sizeY = std::max(200.0f, static_cast<float>(max_y + opts.margin_y));

    /*
     * Split high-fanout connectors (e.g. clk, rst, wide shared buses) into a
     * spatial trunk of smaller connectors (at most 2 local taps per hop, up to
     * 3 on the final hop) sorted left-to-right by sink coordinates so the
     * schematic gets a clean trunk instead of a giant starburst.
     */
    {
        constexpr size_t MAX_SINKS_LEAF = 3;
        constexpr size_t SINKS_PER_HOP = 2;

        std::vector<element *> orig_connectors;
        orig_connectors.reserve(target_scene->elements.size());
        for (auto &elem : target_scene->elements) {
            if (elem && is_connector(elem.get()))
                orig_connectors.push_back(elem.get());
        }

        for (element *conn : orig_connectors) {
            port *cp = conn->port_at(0);
            if (!cp)
                continue;

            std::shared_ptr<wire> drv_wire;
            std::vector<std::pair<std::shared_ptr<wire>, port *>> sink_wires;

            for (const auto &w : cp->get_wires()) {
                if (!w)
                    continue;
                port *other = w->get_other_port(cp);
                if (!other)
                    continue;
                if (other->direction == PORT_OUT && !drv_wire) {
                    drv_wire = w;
                } else {
                    sink_wires.emplace_back(w, other);
                }
            }

            if (sink_wires.size() <= MAX_SINKS_LEAF)
                continue;

            /* Sort sinks spatially left-to-right, then top-to-bottom. */
            std::sort(sink_wires.begin(), sink_wires.end(),
                      [](const auto &a, const auto &b) {
                          if (std::abs(a.second->screenX - b.second->screenX) > 2.0f)
                              return a.second->screenX < b.second->screenX;
                          return a.second->screenY < b.second->screenY;
                      });

            const std::shared_ptr<wire> &tmpl = drv_wire ? drv_wire : sink_wires[0].first;
            const int bus_w = std::max(1, tmpl->get_width());

            /* Keep the first SINKS_PER_HOP sinks on `conn`; chain the rest. */
            port *prev_cp = cp;
            size_t idx = SINKS_PER_HOP;
            while (idx < sink_wires.size()) {
                const size_t remaining = sink_wires.size() - idx;
                const size_t take = (remaining <= MAX_SINKS_LEAF) ? remaining : SINKS_PER_HOP;

                auto next_conn = std::make_unique<connector>(0, 0);
                port *next_cp = next_conn->port_at(0);
                next_cp->set_width(bus_w);

                /* Trunk wire linking previous connector to this new connector. */
                auto trunk = std::make_shared<wire>(0, 0, 0, 0, tmpl->get_color());
                trunk->name = tmpl->name;
                if (!tmpl->get_vcd_name().empty())
                    trunk->set_vcd_name(tmpl->get_vcd_name());
                if (tmpl->get_label_table())
                    trunk->set_label_table(tmpl->get_label_table());
                if (tmpl->value.is_clock) {
                    trunk->value.valid = true;
                    trunk->value.is_clock = true;
                }
                trunk->set_width(bus_w);
                prev_cp->add_wire(trunk);
                next_cp->add_wire(trunk);

                /* Move this chunk's sink wires from the original connector to next_conn. */
                for (size_t k = 0; k < take; ++k) {
                    auto &sw = sink_wires[idx + k].first;
                    cp->remove_wire(sw.get());
                    sw->del_port(cp);
                    next_cp->add_wire(sw);
                }

                target_scene->add_element(std::move(next_conn));
                prev_cp = next_cp;
                idx += take;
            }
        }
    }

    /* Place all connector split junctions clear of components and ports. */
    place_connectors(target_scene);

    /* Update wire endpoints to match the newly placed port coordinates. */
    for (auto &elem : target_scene->elements) {
        for (unsigned int pi = 0;; ++pi) {
            port *p = elem->port_at(pi);
            if (!p)
                break;
            for (const auto &w : p->get_wires()) {
                if (w)
                    w->reseat();
            }
        }
    }
}

void topological_placer::place_connectors(class scene *target_scene)
{
    if (!target_scene || target_scene->elements.empty())
        return;

    /*
     * Build forbidden bounding boxes for every non-connector, non-background
     * element (body + port clearance) so connectors can never overlap a
     * component or sit on top of its pins.
     */
    struct box {
        int x1, y1, x2, y2; /* inclusive */
    };
    std::vector<box> blocked_boxes;
    blocked_boxes.reserve(target_scene->elements.size());

    for (const auto &elem : target_scene->elements) {
        if (!elem || elem->is_background() || is_connector(elem.get()))
            continue;

        const int ox = static_cast<int>(std::floor(elem->get_X()));
        const int oy = static_cast<int>(std::floor(elem->get_Y()));
        const int ow = std::max(1, static_cast<int>(std::ceil(elem->get_width())));
        const int oh = std::max(1, static_cast<int>(std::ceil(elem->get_height())));

        /* Leave 2 cells in X (1 for port + 1 breathing lane) and 1 cell in Y. */
        int x1 = ox - 2;
        int y1 = oy - 1;
        int x2 = ox + ow + 1;
        int y2 = oy + oh;

        /* Also enclose every actual port with a 1-cell halo (covers top/bottom pins). */
        for (unsigned int pi = 0;; ++pi) {
            port *p = elem->port_at(pi);
            if (!p)
                break;
            const int px = static_cast<int>(std::round(p->screenX));
            const int py = static_cast<int>(std::round(p->screenY));
            x1 = std::min(x1, px - 1);
            y1 = std::min(y1, py - 1);
            x2 = std::max(x2, px + 1);
            y2 = std::max(y2, py + 1);
        }

        blocked_boxes.push_back({x1, y1, x2, y2});
    }

    auto hits_component = [&](int x, int y) -> bool {
        for (const auto &b : blocked_boxes) {
            if (x >= b.x1 && x <= b.x2 && y >= b.y1 && y <= b.y2)
                return true;
        }
        return false;
    };

    const int max_x_bound = std::max(200, static_cast<int>(target_scene->sizeX) - 3);
    const int max_y_bound = std::max(200, static_cast<int>(target_scene->sizeY) - 3);

    std::vector<element *> connectors;
    connectors.reserve(target_scene->elements.size());
    for (auto &elem : target_scene->elements) {
        if (elem && is_connector(elem.get()))
            connectors.push_back(elem.get());
    }
    if (connectors.empty())
        return;

    /*
     * Run 3 relaxation passes:
     *   Pass 0: seed each connector from its non-connector neighbors (driver + local sinks).
     *   Passes 1-2: refine using all neighbors (including chained trunk connectors) so
     *               multi-connector trunks align cleanly into straight T-junctions.
     */
    constexpr int NUM_PASSES = 3;
    for (int pass = 0; pass < NUM_PASSES; ++pass) {
        std::set<std::pair<int, int>> occupied_connectors;

        auto near_connector = [&](int x, int y) -> bool {
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    if (occupied_connectors.count({x + dx, y + dy}))
                        return true;
                }
            }
            return false;
        };

        for (element *elem : connectors) {
            port *cp = elem->port_at(0);
            if (!cp)
                continue;

            port *drv_port = nullptr;
            std::vector<port *> sink_ports;
            std::vector<port *> all_ports;
            bool is_ctrl = false;
            bool has_conn_neighbor = false;

            for (const auto &w : cp->get_wires()) {
                if (!w)
                    continue;
                if (w->value.is_clock || w->name == "clk" || w->name == "rst" || w->name == "rst_n")
                    is_ctrl = true;
                for (port *other : w->get_ports()) {
                    if (!other || other == cp)
                        continue;
                    const bool other_is_conn = (other->parent && is_connector(other->parent));
                    if (other_is_conn) {
                        has_conn_neighbor = true;
                        if (pass > 0)
                            all_ports.push_back(other);
                        continue;
                    }
                    all_ports.push_back(other);
                    if (other->direction == PORT_OUT && !drv_port) {
                        drv_port = other;
                    } else {
                        sink_ports.push_back(other);
                    }
                }
            }

            int tx = static_cast<int>(std::round(elem->get_X()));
            int ty = static_cast<int>(std::round(elem->get_Y()));

            if (pass > 0 && has_conn_neighbor && !all_ports.empty()) {
                /* Chained trunk connector: 1D geometric median in X and Y of all neighbors. */
                std::vector<float> xs;
                std::vector<float> ys;
                xs.reserve(all_ports.size());
                ys.reserve(all_ports.size());
                for (port *np : all_ports) {
                    xs.push_back(np->screenX);
                    ys.push_back(np->screenY);
                }
                std::sort(xs.begin(), xs.end());
                std::sort(ys.begin(), ys.end());
                tx = static_cast<int>(std::round(xs[xs.size() / 2]));
                ty = static_cast<int>(std::round(ys[ys.size() / 2]));
            } else if (drv_port && !sink_ports.empty()) {
                if (is_ctrl && !has_conn_neighbor) {
                    /* Single clock/reset split: sit neatly right outside the driver. */
                    tx = static_cast<int>(std::round(drv_port->screenX)) + 3;
                    ty = static_cast<int>(std::round(drv_port->screenY));
                } else {
                    float min_sx = sink_ports[0]->screenX;
                    float sum_sx = 0.0f;
                    std::vector<float> ys;
                    ys.reserve(sink_ports.size() + 1);
                    ys.push_back(drv_port->screenY);
                    for (port *sp : sink_ports) {
                        min_sx = std::min(min_sx, sp->screenX);
                        sum_sx += sp->screenX;
                        ys.push_back(sp->screenY);
                    }
                    const float avg_sx = sum_sx / static_cast<float>(sink_ports.size());
                    if (min_sx > drv_port->screenX + 2.0f) {
                        const int step = std::clamp(
                            static_cast<int>(std::round((min_sx - drv_port->screenX) * 0.5f)),
                            2, 10);
                        tx = static_cast<int>(std::round(drv_port->screenX)) + step;
                    } else {
                        tx = static_cast<int>(std::round((drv_port->screenX + avg_sx) * 0.5f));
                    }

                    /* Geometric 1D median in Y minimizes total vertical wire length. */
                    std::sort(ys.begin(), ys.end());
                    ty = static_cast<int>(std::round(ys[ys.size() / 2]));
                }
            } else if (drv_port) {
                tx = static_cast<int>(std::round(drv_port->screenX)) + 3;
                ty = static_cast<int>(std::round(drv_port->screenY));
            } else if (!sink_ports.empty()) {
                std::vector<float> xs;
                std::vector<float> ys;
                xs.reserve(sink_ports.size());
                ys.reserve(sink_ports.size());
                for (port *sp : sink_ports) {
                    xs.push_back(sp->screenX);
                    ys.push_back(sp->screenY);
                }
                std::sort(xs.begin(), xs.end());
                std::sort(ys.begin(), ys.end());
                /* Place slightly to the left of the local sinks so the trunk enters cleanly. */
                tx = static_cast<int>(std::round(xs[0])) - 3;
                ty = static_cast<int>(std::round(ys[ys.size() / 2]));
            }

            tx = std::clamp(tx, 2, max_x_bound);
            ty = std::clamp(ty, 2, max_y_bound);

            /* Spiral search outward for the nearest valid, non-overlapping grid cell. */
            int best_x = tx;
            int best_y = ty;
            bool found = false;

            auto try_search = [&](bool enforce_conn_spacing, int max_radius) -> bool {
                for (int r = 0; r <= max_radius; ++r) {
                    if (r == 0) {
                        if (!hits_component(tx, ty) &&
                            !(enforce_conn_spacing ? near_connector(tx, ty)
                                                   : occupied_connectors.count({tx, ty}) > 0)) {
                            best_x = tx;
                            best_y = ty;
                            return true;
                        }
                        continue;
                    }
                    /* Walk the Manhattan ring of radius r around (tx, ty). */
                    for (int dx = -r; dx <= r; ++dx) {
                        const int rem = r - std::abs(dx);
                        const int dy_opts[2] = {-rem, rem};
                        const int n_opts = (rem == 0) ? 1 : 2;
                        for (int oi = 0; oi < n_opts; ++oi) {
                            const int cx = tx + dx;
                            const int cy = ty + dy_opts[oi];
                            if (cx < 2 || cy < 2 || cx > max_x_bound || cy > max_y_bound)
                                continue;
                            if (hits_component(cx, cy))
                                continue;
                            if (enforce_conn_spacing ? near_connector(cx, cy)
                                                     : occupied_connectors.count({cx, cy}) > 0)
                                continue;
                            best_x = cx;
                            best_y = cy;
                            return true;
                        }
                    }
                }
                return false;
            };

            found = try_search(/*enforce_conn_spacing=*/true, 20);
            if (!found)
                found = try_search(/*enforce_conn_spacing=*/false, 80);

            if (!found) {
                best_x = std::max(2, tx);
                best_y = std::max(2, ty);
                while (occupied_connectors.count({best_x, best_y}) || hits_component(best_x, best_y))
                    best_y++;
            }

            occupied_connectors.insert({best_x, best_y});
            elem->place(best_x, best_y);
            elem->reseat();

            if (pass == NUM_PASSES - 1) {
                for (const auto &w : cp->get_wires()) {
                    if (w) {
                        w->reseat();
                        w->clear_route();
                    }
                }
                target_scene->sizeX = std::max(target_scene->sizeX, static_cast<float>(best_x + 6));
                target_scene->sizeY = std::max(target_scene->sizeY, static_cast<float>(best_y + 6));
            }
        }
    }
}
