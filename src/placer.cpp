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

bool is_primary_output(const element *elem)
{
    return const_cast<element *>(elem)->class_id() == "model_output:";
}

bool is_sequential(const element *elem)
{
    const std::string cid = const_cast<element *>(elem)->class_id();
    return cid == "model_reg:" || cid == "model_dflipflop:" || cid == "model_deflipflop:";
}

bool is_connector(const element *elem)
{
    return const_cast<element *>(elem)->class_id() == "connector:";
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
     * transparently through connector split junctions:
     * - all_out/all_in: full netlist edges (used for barycenter ordering)
     * - dag_out: combinational edges with register-output edges omitted so
     *   sequential feedback loops (reg.q -> add -> reg.d) don't form cycles.
     */
    std::vector<std::vector<size_t>> all_out(n), all_in(n);
    std::vector<std::vector<size_t>> dag_out(n);

    for (size_t u = 0; u < n; ++u) {
        element *src_elem = target_scene->elements[u].get();
        if (is_connector(src_elem))
            continue;

        const bool src_is_seq = is_sequential(src_elem);
        const bool src_is_const = is_constant(src_elem);

        for (unsigned int pi = 0;; ++pi) {
            port *p = src_elem->port_at(pi);
            if (!p)
                break;
            if (p->direction != PORT_OUT)
                continue;

            std::vector<size_t> sinks;
            collect_sink_elements(p, elem_index, sinks);
            for (size_t v : sinks) {
                if (v == u)
                    continue;
                all_out[u].push_back(v);
                all_in[v].push_back(u);

                if (!src_is_seq && !src_is_const &&
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

    /* Longest-path topological layering via Kahn's algorithm. */
    std::vector<int> layer(n, 0);
    std::queue<size_t> q;
    for (size_t i = 0; i < n; ++i) {
        element *e = target_scene->elements[i].get();
        if (is_connector(e))
            continue;
        if (is_primary_input(e)) {
            layer[i] = 0;
        } else if (!is_constant(e) && !is_primary_output(e)) {
            layer[i] = 1;
        }
        if (indegree[i] == 0)
            q.push(i);
    }

    while (!q.empty()) {
        size_t u = q.front();
        q.pop();
        for (size_t v : dag_out[u]) {
            if (layer[v] < layer[u] + 1)
                layer[v] = layer[u] + 1;
            if (--indegree[v] == 0)
                q.push(v);
        }
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
     * Order nodes within each column using predecessor barycenter (average Y
     * position of already-assigned predecessors) to reduce wire crossings.
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

    /*
     * Compute column X coordinates and element Y coordinates.
     * Split tall columns (> max_elems_per_subcol) into multiple sub-columns so
     * large modules stay widescreen and give A* routing generous horizontal channels.
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

            for (size_t idx = begin; idx < end; ++idx) {
                element *elem = target_scene->elements[col[idx]].get();
                int w = std::max(1, static_cast<int>(std::ceil(elem->get_width())));
                int h = std::max(1, static_cast<int>(std::ceil(elem->get_height())));
                col_width = std::max(col_width, w);

                elem->place(cur_x, cur_y);
                elem->reseat();

                cur_y += h + opts.row_gap;
            }

            max_y = std::max(max_y, cur_y);
            /* Scale horizontal routing channel width with column density. */
            const int density_bonus = std::min(16, static_cast<int>((end - begin) / 2));
            cur_x += col_width + opts.col_gap + density_bonus;
            max_x = std::max(max_x, cur_x);
        }
    }

    /*
     * Place connector split junctions staggered across the horizontal routing
     * channel to the right of their driving port so they never form a wall.
     */
    std::set<std::pair<int, int>> occupied_connectors;
    std::unordered_map<int, int> channel_stagger;
    for (size_t i = 0; i < n; ++i) {
        element *elem = target_scene->elements[i].get();
        if (!is_connector(elem))
            continue;

        port *cp = elem->port_at(0);
        if (!cp)
            continue;

        port *drv_port = nullptr;
        float sink_y_sum = 0.0f;
        int sink_cnt = 0;

        for (const auto &w : cp->get_wires()) {
            if (!w)
                continue;
            for (port *other : w->get_ports()) {
                if (!other || other == cp)
                    continue;
                if (other->direction == PORT_OUT) {
                    drv_port = other;
                } else if (other->direction == PORT_IN) {
                    sink_y_sum += other->screenY;
                    sink_cnt++;
                }
            }
        }

        int cx = opts.margin_x;
        int cy = opts.margin_y;
        if (drv_port) {
            const int base_x = static_cast<int>(std::round(drv_port->screenX));
            const int lane = (channel_stagger[base_x]++) % std::max(1, opts.col_gap - 4);
            cx = base_x + 2 + lane;
            cy = (sink_cnt > 0) ? static_cast<int>(std::round(sink_y_sum / sink_cnt))
                                : static_cast<int>(std::round(drv_port->screenY));
        }
        cx = std::max(2, cx);
        cy = std::max(2, cy);

        while (occupied_connectors.count({cx, cy}) ||
               !target_scene->can_place_element(cx, cy, 1, 1, elem)) {
            cy++;
        }
        occupied_connectors.insert({cx, cy});

        elem->place(cx, cy);
        elem->reseat();
        max_x = std::max(max_x, cx + 2);
        max_y = std::max(max_y, cy + 2);
    }

    /* Ensure the scene grid comfortably encloses all elements and their ports. */
    target_scene->sizeX = std::max(200.0f, static_cast<float>(max_x + opts.margin_x));
    target_scene->sizeY = std::max(200.0f, static_cast<float>(max_y + opts.margin_y));

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
