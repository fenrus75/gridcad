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

                /* Keep sequential forward edges; true cycles are broken selectively via SCC below. */
                if (!src_is_const && !is_primary_output(target_scene->elements[v].get())) {
                    dag_out[u].push_back(v);
                }
            }
        }
    }

    auto dedup = [](std::vector<size_t> &vec) {
        std::sort(vec.begin(), vec.end());
        vec.erase(std::unique(vec.begin(), vec.end()), vec.end());
    };
    for (size_t u = 0; u < n; ++u) {
        dedup(all_out[u]);
        dedup(all_in[u]);
        dedup(dag_out[u]);
    }

    /*
     * Virtual hierarchy: group non-I/O, non-constant elements that share the
     * same Verilog `src_location` AND are connected to each other into a
     * temporary virtual super-block. Each multi-element virtual block runs a
     * compact local left-to-right PnR to compute relative offsets and a bounding
     * box; top-level PnR then places virtual blocks, after which the grouping
     * is dissolved into real element coordinates.
     */
    struct vgroup {
        std::vector<size_t> members; /* indices into target_scene->elements */
        std::vector<int> rel_x;
        std::vector<int> rel_y;
        int width = 1;
        int height = 1;
        bool is_input = false;
        bool is_output = false;
        bool is_const = false;
        bool is_slice_grp = false;
        bool is_clock = false;
        bool is_reset = false;
        bool is_seq = false;
    };

    std::vector<size_t> elem_to_group(n, SIZE_MAX);
    std::vector<vgroup> groups;
    groups.reserve(n);

    /* Bucket eligible elements by src_location. */
    std::unordered_map<std::string, std::vector<size_t>> src_buckets;
    for (size_t i = 0; i < n; ++i) {
        element *e = target_scene->elements[i].get();
        if (is_connector(e) || is_primary_input(e) || is_primary_output(e) || is_constant(e))
            continue;
        const std::string &src = e->get_src_location();
        if (!src.empty())
            src_buckets[src].push_back(i);
    }

    /* Find connected components inside each src_location bucket of size >= 2. */
    std::vector<bool> grouped(n, false);
    for (const auto &[src, bucket] : src_buckets) {
        if (bucket.size() < 2)
            continue;
        std::unordered_set<size_t> in_bucket(bucket.begin(), bucket.end());
        std::unordered_set<size_t> visited;

        for (size_t start : bucket) {
            if (visited.count(start))
                continue;
            std::vector<size_t> comp;
            std::queue<size_t> bfs;
            bfs.push(start);
            visited.insert(start);
            while (!bfs.empty()) {
                size_t cur = bfs.front();
                bfs.pop();
                comp.push_back(cur);
                for (size_t nb : all_out[cur]) {
                    if (in_bucket.count(nb) && !visited.count(nb)) {
                        visited.insert(nb);
                        bfs.push(nb);
                    }
                }
                for (size_t nb : all_in[cur]) {
                    if (in_bucket.count(nb) && !visited.count(nb)) {
                        visited.insert(nb);
                        bfs.push(nb);
                    }
                }
            }

            if (comp.size() < 2)
                continue;

            /* Local left-to-right PnR inside this virtual block. */
            const size_t k = comp.size();
            std::unordered_map<size_t, size_t> local_idx;
            local_idx.reserve(k);
            for (size_t li = 0; li < k; ++li)
                local_idx[comp[li]] = li;

            std::vector<std::vector<size_t>> l_out(k);
            std::vector<int> l_indeg(k, 0);
            for (size_t li = 0; li < k; ++li) {
                for (size_t dst : all_out[comp[li]]) {
                    auto it = local_idx.find(dst);
                    if (it != local_idx.end() && it->second != li) {
                        l_out[li].push_back(it->second);
                        l_indeg[it->second]++;
                    }
                }
            }

            std::vector<int> l_col(k, 0);
            std::vector<bool> l_done(k, false);
            std::queue<size_t> lq;
            size_t l_done_cnt = 0;
            for (size_t li = 0; li < k; ++li) {
                if (l_indeg[li] == 0)
                    lq.push(li);
            }
            auto drain_local = [&]() {
                while (!lq.empty()) {
                    size_t u = lq.front();
                    lq.pop();
                    if (l_done[u])
                        continue;
                    l_done[u] = true;
                    l_done_cnt++;
                    for (size_t v : l_out[u]) {
                        if (l_col[v] < l_col[u] + 1)
                            l_col[v] = l_col[u] + 1;
                        if (--l_indeg[v] == 0)
                            lq.push(v);
                    }
                }
            };
            drain_local();
            while (l_done_cnt < k) {
                size_t best = k;
                int best_in = INT_MAX;
                for (size_t li = 0; li < k; ++li) {
                    if (!l_done[li] && l_indeg[li] < best_in) {
                        best_in = l_indeg[li];
                        best = li;
                    }
                }
                if (best == k)
                    break;
                l_indeg[best] = 0;
                lq.push(best);
                drain_local();
            }

            int max_lc = 0;
            for (int c : l_col)
                max_lc = std::max(max_lc, c);
            std::vector<std::vector<size_t>> l_cols(static_cast<size_t>(max_lc + 1));
            for (size_t li = 0; li < k; ++li)
                l_cols[static_cast<size_t>(l_col[li])].push_back(li);

            vgroup vg;
            vg.members = comp;
            vg.rel_x.assign(k, 0);
            vg.rel_y.assign(k, 0);

            constexpr int INNER_COL_GAP = 5;
            constexpr int INNER_ROW_GAP = 2;
            int lx = 0;
            int max_w = 1;
            int max_h = 1;
            for (int c = 0; c <= max_lc; ++c) {
                const auto &col = l_cols[static_cast<size_t>(c)];
                if (col.empty())
                    continue;
                int ly = 0;
                int cw = 1;
                for (size_t li : col) {
                    element *e = target_scene->elements[comp[li]].get();
                    const int ew = std::max(1, static_cast<int>(std::ceil(e->get_width())));
                    const int eh = std::max(1, static_cast<int>(std::ceil(e->get_height())));
                    vg.rel_x[li] = lx;
                    vg.rel_y[li] = ly;
                    cw = std::max(cw, ew);
                    ly += eh + INNER_ROW_GAP;
                }
                max_w = std::max(max_w, lx + cw);
                max_h = std::max(max_h, std::max(1, ly - INNER_ROW_GAP));
                lx += cw + INNER_COL_GAP;
            }
            vg.width = max_w;
            vg.height = max_h;
            for (size_t idx : comp) {
                if (is_cycle_breaker(target_scene->elements[idx].get()))
                    vg.is_seq = true;
            }

            const size_t gid = groups.size();
            for (size_t idx : comp) {
                grouped[idx] = true;
                elem_to_group[idx] = gid;
            }
            groups.push_back(std::move(vg));
        }
    }

    /*
     * Identify single-sink constants (directly feeding one input port with no
     * connector split). These will be tucked directly beside their consumer's
     * input pin after main placement instead of occupying column slots.
     */
    std::vector<bool> tucked_const(n, false);
    std::vector<std::pair<size_t, port *>> single_sink_consts;
    for (size_t i = 0; i < n; ++i) {
        element *e = target_scene->elements[i].get();
        if (!is_constant(e))
            continue;
        port *cp = e->port_at(0);
        if (!cp || cp->get_wires().size() != 1)
            continue;
        const auto &w = cp->get_wires()[0];
        if (!w)
            continue;
        port *other = w->get_other_port(cp);
        if (other && other->parent && !is_connector(other->parent)) {
            tucked_const[i] = true;
            single_sink_consts.emplace_back(i, other);
        }
    }

    /* Remaining non-connector elements become singleton virtual groups. */
    for (size_t i = 0; i < n; ++i) {
        element *e = target_scene->elements[i].get();
        if (is_connector(e) || grouped[i] || tucked_const[i])
            continue;
        vgroup vg;
        vg.members.push_back(i);
        vg.rel_x.push_back(0);
        vg.rel_y.push_back(0);
        vg.width = std::max(1, static_cast<int>(std::ceil(e->get_width())));
        vg.height = std::max(1, static_cast<int>(std::ceil(e->get_height())));
        vg.is_input = is_primary_input(e);
        vg.is_output = is_primary_output(e);
        vg.is_const = is_constant(e);
        vg.is_slice_grp = is_slice(e);
        vg.is_clock = is_clock_element(e);
        vg.is_reset = is_reset_element(e);
        vg.is_seq = is_cycle_breaker(e);
        elem_to_group[i] = groups.size();
        groups.push_back(std::move(vg));
    }

    /* Contract element-level graphs onto virtual groups. */
    const size_t ng = groups.size();
    std::vector<std::vector<size_t>> g_all_out(ng), g_all_in(ng), g_dag_out(ng);
    for (size_t u = 0; u < n; ++u) {
        const size_t gu = elem_to_group[u];
        if (gu == SIZE_MAX)
            continue;
        for (size_t v : all_out[u]) {
            const size_t gv = elem_to_group[v];
            if (gv != SIZE_MAX && gv != gu) {
                g_all_out[gu].push_back(gv);
                g_all_in[gv].push_back(gu);
            }
        }
        for (size_t v : dag_out[u]) {
            const size_t gv = elem_to_group[v];
            if (gv != SIZE_MAX && gv != gu)
                g_dag_out[gu].push_back(gv);
        }
    }
    for (size_t gu = 0; gu < ng; ++gu) {
        dedup(g_all_out[gu]);
        dedup(g_all_in[gu]);
        dedup(g_dag_out[gu]);
    }

    /*
     * Smart cycle breaking: instead of cutting every register's output (which
     * reverses feed-forward pipelines like add -> reg -> mux -> reg -> out),
     * find actual Strongly Connected Components (Tarjan's SCC) and break each
     * cycle by cutting only the SCC-internal outgoing edges of the sequential
     * node closest to a primary output.
     */
    std::vector<int> dist_to_out(ng, INT_MAX);
    {
        std::queue<size_t> rq;
        for (size_t i = 0; i < ng; ++i) {
            if (groups[i].is_output) {
                dist_to_out[i] = 0;
                rq.push(i);
            }
        }
        while (!rq.empty()) {
            size_t v = rq.front();
            rq.pop();
            for (size_t u : g_all_in[v]) {
                if (dist_to_out[u] == INT_MAX) {
                    dist_to_out[u] = dist_to_out[v] + 1;
                    rq.push(u);
                }
            }
        }
    }

    {
        /* Iterative Tarjan's SCC + selective back-edge cutting until acyclic. */
        std::vector<int> disc(ng, -1);
        std::vector<int> low(ng, 0);
        std::vector<bool> on_stack(ng, false);
        std::vector<size_t> tarjan_stack;
        tarjan_stack.reserve(ng);
        std::vector<bool> in_scc(ng, false);

        for (size_t guard = 0; guard < ng; ++guard) {
            std::fill(disc.begin(), disc.end(), -1);
            std::fill(on_stack.begin(), on_stack.end(), false);
            tarjan_stack.clear();
            int timer = 0;
            std::vector<std::vector<size_t>> cyclic_sccs;

            /* Iterative DFS for Tarjan's SCC to avoid deep recursion on large netlists. */
            struct frame {
                size_t u;
                size_t edge_idx;
            };
            std::vector<frame> call_stack;
            call_stack.reserve(ng);

            for (size_t start = 0; start < ng; ++start) {
                if (disc[start] != -1)
                    continue;
                disc[start] = low[start] = timer++;
                tarjan_stack.push_back(start);
                on_stack[start] = true;
                call_stack.push_back({start, 0});

                while (!call_stack.empty()) {
                    auto &top = call_stack.back();
                    size_t u = top.u;
                    if (top.edge_idx < g_dag_out[u].size()) {
                        size_t v = g_dag_out[u][top.edge_idx++];
                        if (disc[v] == -1) {
                            disc[v] = low[v] = timer++;
                            tarjan_stack.push_back(v);
                            on_stack[v] = true;
                            call_stack.push_back({v, 0});
                        } else if (on_stack[v]) {
                            low[u] = std::min(low[u], disc[v]);
                        }
                    } else {
                        if (low[u] == disc[u]) {
                            std::vector<size_t> scc;
                            while (true) {
                                size_t w = tarjan_stack.back();
                                tarjan_stack.pop_back();
                                on_stack[w] = false;
                                scc.push_back(w);
                                if (w == u)
                                    break;
                            }
                            if (scc.size() > 1)
                                cyclic_sccs.push_back(std::move(scc));
                        }
                        call_stack.pop_back();
                        if (!call_stack.empty()) {
                            size_t parent = call_stack.back().u;
                            low[parent] = std::min(low[parent], low[u]);
                        }
                    }
                }
            }

            if (cyclic_sccs.empty())
                break;

            for (const auto &scc : cyclic_sccs) {
                for (size_t u : scc)
                    in_scc[u] = true;

                /* Prefer sequential node closest to a primary output, breaking ties by min SCC out-degree. */
                size_t best_u = scc[0];
                bool best_seq = false;
                int best_dist = INT_MAX;
                size_t best_deg = SIZE_MAX;

                for (size_t u : scc) {
                    size_t scc_out_deg = 0;
                    for (size_t v : g_dag_out[u]) {
                        if (in_scc[v])
                            scc_out_deg++;
                    }
                    const bool is_s = groups[u].is_seq;
                    const int d = dist_to_out[u];
                    bool better = false;
                    if (is_s != best_seq)
                        better = is_s;
                    else if (d != best_dist)
                        better = (d < best_dist);
                    else if (scc_out_deg != best_deg)
                        better = (scc_out_deg < best_deg);
                    if (better) {
                        best_u = u;
                        best_seq = is_s;
                        best_dist = d;
                        best_deg = scc_out_deg;
                    }
                }

                /* Cut all SCC-internal outgoing edges from best_u. */
                auto &outs = g_dag_out[best_u];
                outs.erase(std::remove_if(outs.begin(), outs.end(),
                                          [&](size_t v) { return in_scc[v]; }),
                           outs.end());

                for (size_t u : scc)
                    in_scc[u] = false;
            }
        }
    }

    std::vector<int> indegree(ng, 0);
    for (size_t gu = 0; gu < ng; ++gu) {
        for (size_t gv : g_dag_out[gu])
            indegree[gv]++;
    }

    /* Pass 1 (ASAP, left-to-right): longest-path topological layering via Kahn's + cyclic peel. */
    std::vector<int> layer(ng, 0);
    std::vector<bool> processed(ng, false);
    std::vector<size_t> topo_order;
    topo_order.reserve(ng);
    std::queue<size_t> q;
    size_t processed_count = 0;

    for (size_t i = 0; i < ng; ++i) {
        if (groups[i].is_input) {
            layer[i] = 0;
        } else if (!groups[i].is_const && !groups[i].is_output) {
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
            topo_order.push_back(u);
            for (size_t v : g_dag_out[u]) {
                if (layer[v] < layer[u] + 1)
                    layer[v] = layer[u] + 1;
                if (--indegree[v] == 0)
                    q.push(v);
            }
        }
    };

    drain_queue();

    /* If any combinational/submodule cycle remained, greedily break the lowest-indegree node. */
    while (processed_count < ng) {
        size_t best = ng;
        int best_in = INT_MAX;
        for (size_t i = 0; i < ng; ++i) {
            if (!processed[i] && indegree[i] < best_in) {
                best_in = indegree[i];
                best = i;
            }
        }
        if (best == ng)
            break;
        indegree[best] = 0;
        q.push(best);
        drain_queue();
    }

    /* Pin primary outputs in the rightmost column before running the right-to-left ALAP pass. */
    int max_internal_layer = 0;
    for (size_t i = 0; i < ng; ++i) {
        if (!groups[i].is_output)
            max_internal_layer = std::max(max_internal_layer, layer[i]);
    }
    const int output_layer = max_internal_layer + 1;
    for (size_t i = 0; i < ng; ++i) {
        if (groups[i].is_output)
            layer[i] = output_layer;
    }

    /*
     * Pass 2 (ALAP, right-to-left): walk reverse topological order and pull
     * internal groups rightward toward their earliest forward consumer so
     * short combinational cones sit next to what consumes them instead of
     * piling up on the left edge.
     */
    for (auto it = topo_order.rbegin(); it != topo_order.rend(); ++it) {
        const size_t u = *it;
        if (groups[u].is_input || groups[u].is_output || groups[u].is_const)
            continue;

        int min_succ_layer = INT_MAX;
        for (size_t v : g_dag_out[u]) {
            if (layer[v] > layer[u])
                min_succ_layer = std::min(min_succ_layer, layer[v]);
        }
        for (size_t v : g_all_out[u]) {
            if (groups[v].is_output)
                min_succ_layer = std::min(min_succ_layer, layer[v]);
        }
        if (min_succ_layer < INT_MAX)
            layer[u] = std::max(layer[u], min_succ_layer - 1);
    }

    /* Place constants directly in the column preceding their earliest consumer. */
    for (size_t i = 0; i < ng; ++i) {
        if (!groups[i].is_const)
            continue;
        int min_sink_layer = 1;
        bool has_sink = false;
        for (size_t v : g_all_out[i]) {
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
    for (size_t i = 0; i < ng; ++i) {
        if (!groups[i].is_slice_grp)
            continue;
        int min_sink_layer = INT_MAX;
        for (size_t v : g_all_out[i]) {
            if (!groups[v].is_output)
                min_sink_layer = std::min(min_sink_layer, layer[v]);
        }
        if (min_sink_layer < INT_MAX)
            layer[i] = std::max(1, min_sink_layer - 1);
    }

    int num_layers = output_layer + 1;
    std::vector<std::vector<size_t>> columns(static_cast<size_t>(num_layers));
    for (size_t i = 0; i < ng; ++i)
        columns[static_cast<size_t>(layer[i])].push_back(i);

    /*
     * Order Column 0 inputs so data inputs sit at the top, reset below them,
     * and clk at the very bottom (best routing geometry for clock trees).
     */
    if (!columns.empty()) {
        std::stable_sort(columns[0].begin(), columns[0].end(),
                         [&](size_t a, size_t b) {
                             auto rank = [&](size_t idx) -> int {
                                 if (groups[idx].is_clock)
                                     return 2;
                                 if (groups[idx].is_reset)
                                     return 1;
                                 return 0;
                             };
                             return rank(a) < rank(b);
                         });
    }

    /*
     * Two-pass barycenter ordering (forward then backward) to align connected
     * slices, LUTs, virtual groups, and submodules vertically and minimize crossings.
     */
    std::vector<float> y_est(ng, 0.0f);
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
            for (size_t pred : g_all_in[u]) {
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
            for (size_t succ : g_all_out[u]) {
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
     * Compute column X coordinates and virtual group Y coordinates, then
     * dissolve each virtual group into real element coordinates.
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
                const vgroup &vg = groups[col[idx]];
                const int w = vg.width;
                const int h = vg.height;
                const int x_off = (do_stagger && ((idx - begin) % 2 == 1)) ? opts.stagger_x : 0;
                col_width = std::max(col_width, w + x_off);

                for (size_t mi = 0; mi < vg.members.size(); ++mi) {
                    element *elem = target_scene->elements[vg.members[mi]].get();
                    elem->place(cur_x + x_off + vg.rel_x[mi], cur_y + vg.rel_y[mi]);
                    elem->reseat();
                }

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
     * Tuck single-sink constants directly beside the specific input pin they
     * feed (left of left-side pins, upper-left of top pins like reg E / mux S),
     * searching outward for the closest collision-free cell.
     */
    if (!single_sink_consts.empty()) {
        struct cbox {
            int x1, y1, x2, y2; /* inclusive */
        };
        std::vector<cbox> occupied;
        occupied.reserve(n);
        for (size_t i = 0; i < n; ++i) {
            element *e = target_scene->elements[i].get();
            if (!e || e->is_background() || is_connector(e) || tucked_const[i])
                continue;
            const int ox = static_cast<int>(std::floor(e->get_X()));
            const int oy = static_cast<int>(std::floor(e->get_Y()));
            const int ow = std::max(1, static_cast<int>(std::ceil(e->get_width())));
            const int oh = std::max(1, static_cast<int>(std::ceil(e->get_height())));
            occupied.push_back({ox - 1, oy - 1, ox + ow, oy + oh});
        }

        auto overlaps_any = [&](int x, int y, int w, int h) -> bool {
            const int ax1 = x;
            const int ay1 = y;
            const int ax2 = x + w; /* include right output pin */
            const int ay2 = y + h - 1;
            for (const auto &b : occupied) {
                if (ax1 <= b.x2 && ax2 >= b.x1 && ay1 <= b.y2 && ay2 >= b.y1)
                    return true;
            }
            return false;
        };

        const int max_cx = std::max(200, static_cast<int>(target_scene->sizeX) - 4);
        const int max_cy = std::max(200, static_cast<int>(target_scene->sizeY) - 4);

        for (const auto &[ci, sp] : single_sink_consts) {
            element *ce = target_scene->elements[ci].get();
            const int cw = std::max(1, static_cast<int>(std::ceil(ce->get_width())));
            const int ch = std::max(1, static_cast<int>(std::ceil(ce->get_height())));
            const int px = static_cast<int>(std::round(sp->screenX));
            const int py = static_cast<int>(std::round(sp->screenY));

            int tx = px - cw - 2;
            int ty = py;
            if (sp->parent) {
                const float parent_y = sp->parent->get_Y();
                const float parent_h = sp->parent->get_height();
                if (sp->screenY < parent_y) {
                    /* Top pin (e.g. reg E, mux S): tuck just above & left. */
                    tx = px - cw - 2;
                    ty = py - 1;
                } else if (sp->screenY >= parent_y + parent_h) {
                    /* Bottom pin: tuck just below & left. */
                    tx = px - cw - 2;
                    ty = py + 1;
                }
            }
            tx = std::clamp(tx, 2, max_cx - cw);
            ty = std::clamp(ty, 2, max_cy - ch);

            int best_x = tx;
            int best_y = ty;
            bool found = false;
            for (int r = 0; r <= 30 && !found; ++r) {
                if (r == 0) {
                    if (!overlaps_any(tx, ty, cw, ch)) {
                        best_x = tx;
                        best_y = ty;
                        found = true;
                    }
                    continue;
                }
                for (int dx = -r; dx <= r && !found; ++dx) {
                    const int rem = r - std::abs(dx);
                    const int dy_opts[2] = {-rem, rem};
                    const int n_opts = (rem == 0) ? 1 : 2;
                    for (int oi = 0; oi < n_opts; ++oi) {
                        const int cx = tx + dx;
                        const int cy = ty + dy_opts[oi];
                        if (cx < 2 || cy < 2 || cx + cw > max_cx || cy + ch > max_cy)
                            continue;
                        if (!overlaps_any(cx, cy, cw, ch)) {
                            best_x = cx;
                            best_y = cy;
                            found = true;
                            break;
                        }
                    }
                }
            }

            ce->place(best_x, best_y);
            ce->reseat();
            occupied.push_back({best_x - 1, best_y - 1, best_x + cw + 1, best_y + ch});
            max_x = std::max(max_x, best_x + cw + 2);
            max_y = std::max(max_y, best_y + ch + 2);
        }

        target_scene->sizeX = std::max(target_scene->sizeX, static_cast<float>(max_x + opts.margin_x));
        target_scene->sizeY = std::max(target_scene->sizeY, static_cast<float>(max_y + opts.margin_y));
    }

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
                if (!tmpl->get_src_location().empty() || !tmpl->get_src_verilog().empty())
                    trunk->set_src_info(tmpl->get_src_location(), tmpl->get_src_verilog());
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

            const int min_x_bound = drv_port
                                        ? std::min(max_x_bound,
                                                   std::max(2, static_cast<int>(std::round(drv_port->screenX)) + 1))
                                        : 2;

            tx = std::clamp(tx, drv_port ? std::min(max_x_bound, min_x_bound + 1) : min_x_bound, max_x_bound);
            ty = std::clamp(ty, 2, max_y_bound);

            /* Spiral search outward for the nearest valid, non-overlapping grid cell. */
            int best_x = tx;
            int best_y = ty;
            bool found = false;

            auto try_search = [&](bool enforce_conn_spacing, int max_radius) -> bool {
                for (int r = 0; r <= max_radius; ++r) {
                    if (r == 0) {
                        if (tx >= min_x_bound && !hits_component(tx, ty) &&
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
                            if (cx < min_x_bound || cy < 2 || cx > max_x_bound || cy > max_y_bound)
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
                best_x = std::max(min_x_bound, tx);
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
