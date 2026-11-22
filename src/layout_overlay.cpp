/*
 * gridcad -- a tool to do digital circuit design with a GUI
 *
 * (C) Copyright 2025  -- Arjan van de Ven
 *
 * Licensed under the terms of the
 * GNU General Public LIcense version 3.0
 */

#include "layout_overlay.h"
#include "gridcad.h"
#include "scene.h"
#include "element.h"
#include "connector.h"
#include "port.h"
#include "wire.h"
#include "model_label.h"
#include "model_datascope.h"
#include "model_nest.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <format>
#include <fstream>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using json = nlohmann::json;

namespace {

/* Deterministic 64-bit FNV-1a hash formatted as 16 lowercase hex digits. */
std::string fnv1a_hex(std::string_view s)
{
    uint64_t h = 14695981039346656037ULL;
    for (unsigned char c : s) {
        h ^= static_cast<uint64_t>(c);
        h *= 1099511628211ULL;
    }
    return std::format("{:016x}", h);
}

/* Sanitize untrusted module name for safe single-component filename (no path traversal). */
std::string sanitize_filename_component(std::string_view name)
{
    std::string out;
    out.reserve(name.size());
    for (unsigned char ch : name) {
        if (std::isalnum(ch) || ch == '_' || ch == '-' || ch == '.')
            out.push_back(static_cast<char>(ch));
        else
            out.push_back('_');
    }
    while (!out.empty() && out.front() == '.')
        out.erase(out.begin());
    if (out.empty() || out == "." || out == "..")
        return "unnamed";
    return out;
}

std::filesystem::path cache_root_dir(void)
{
    if (const char *xdg = std::getenv("XDG_CACHE_HOME"); xdg && xdg[0] != '\0')
        return std::filesystem::path(xdg) / "gridcad";
    if (const char *home = std::getenv("HOME"); home && home[0] != '\0')
        return std::filesystem::path(home) / ".cache" / "gridcad";
    return std::filesystem::temp_directory_path() / "gridcad";
}

/* Return first non-empty output wire name on an element (used for fallback matching). */
std::string first_out_wire_name(element *e)
{
    if (!e)
        return "";
    for (unsigned int pi = 0;; ++pi) {
        port *p = e->port_at(pi);
        if (!p)
            break;
        if (p->direction != PORT_OUT)
            continue;
        for (const auto &w : p->get_wires()) {
            if (w && !w->name.empty())
                return w->name;
        }
    }
    return "";
}

bool is_real_verilog_wire_name(std::string_view nm)
{
    return !nm.empty() && !nm.starts_with("wire_") && !nm.starts_with("Wire-");
}

/*
 * Compute a stable key for a wire's net:
 *   - Named Verilog wire: "sig:<name>" (or "vcd:<vcd_name>")
 *   - Unnamed internal wire: "drv:<driver_class>:<driver_name>:<driver_pin>"
 */
std::string compute_net_key_from_port(port *start_port)
{
    if (!start_port)
        return "";

    std::unordered_set<const wire *> seen_wires;
    std::unordered_set<const port *> seen_ports;
    std::queue<port *> q;
    q.push(start_port);
    seen_ports.insert(start_port);

    std::string named_sig;
    std::string vcd_sig;
    std::string drv_key;

    if (start_port->direction == PORT_OUT && start_port->parent &&
        start_port->parent->class_id() != "connector:") {
        drv_key = std::format("drv:{}:{}:{}",
                              start_port->parent->class_id(),
                              start_port->parent->get_name(),
                              start_port->name);
    }

    while (!q.empty()) {
        port *cur = q.front();
        q.pop();
        for (const auto &w : cur->get_wires()) {
            if (!w || !seen_wires.insert(w.get()).second)
                continue;
            if (vcd_sig.empty() && !w->get_vcd_name().empty())
                vcd_sig = w->get_vcd_name();
            if (named_sig.empty() && is_real_verilog_wire_name(w->name))
                named_sig = w->name;
            for (port *ep : w->get_ports()) {
                if (!ep || !seen_ports.insert(ep).second)
                    continue;
                if (ep->parent && ep->parent->class_id() == "connector:") {
                    q.push(ep);
                } else {
                    if (drv_key.empty() && ep->direction == PORT_OUT && ep->parent) {
                        drv_key = std::format("drv:{}:{}:{}",
                                              ep->parent->class_id(),
                                              ep->parent->get_name(),
                                              ep->name);
                    }
                    /* Also continue through a non-connector port (e.g. scope wired directly to a pin). */
                    if (ep->parent && ep->parent->class_id() != "model_datascope:")
                        q.push(ep);
                }
            }
        }
    }

    if (!named_sig.empty())
        return "sig:" + named_sig;
    if (!vcd_sig.empty())
        return "vcd:" + vcd_sig;
    return drv_key;
}

std::string compute_net_key_from_wire(wire *w)
{
    if (!w)
        return "";
    if (is_real_verilog_wire_name(w->name))
        return "sig:" + w->name;
    if (!w->get_vcd_name().empty())
        return "vcd:" + w->get_vcd_name();
    for (port *ep : w->get_ports()) {
        if (ep)
            return compute_net_key_from_port(ep);
    }
    return "";
}

} // namespace

std::filesystem::path layout_overlay::layout_dir_for_project(std::string_view project_key)
{
    const std::string key = project_key.empty() ? std::string("default") : std::string(project_key);
    return cache_root_dir() / ("project_" + fnv1a_hex(key)) / "layouts";
}

std::filesystem::path layout_overlay::layout_file_for_module(std::string_view project_key,
                                                             std::string_view module_name)
{
    return layout_dir_for_project(project_key) / (sanitize_filename_component(module_name) + ".json");
}

bool layout_overlay::save_scene_to_file(class scene *target_scene, const std::filesystem::path &path)
{
    if (!target_scene)
        return false;

    json root;
    root["version"] = 1;
    root["module"] = target_scene->get_layout_module_name();

    json j_elems = json::array();
    json j_labels = json::array();
    json j_scopes = json::array();
    json j_wire_colors = json::object();

    std::unordered_set<const wire *> seen_wires;

    for (const auto &elem : target_scene->elements) {
        if (!elem)
            continue;
        const std::string cid = elem->class_id();
        if (cid == "connector:")
            continue;

        if (cid == "model_label:") {
            auto *lbl = static_cast<model_label *>(elem.get());
            json jl;
            jl["name"] = lbl->get_name();
            jl["X"] = lbl->get_X();
            jl["Y"] = lbl->get_Y();
            jl["sizeX"] = static_cast<int>(std::round(lbl->get_width()));
            jl["sizeY"] = static_cast<int>(std::round(lbl->get_height()));
            jl["color"] = lbl->get_color();
            jl["bottomlabel"] = lbl->get_bottomlabel();
            j_labels.push_back(std::move(jl));
            continue;
        }

        if (cid == "model_datascope:") {
            auto *ds = static_cast<model_datascope *>(elem.get());
            json jd;
            jd["name"] = ds->get_name();
            jd["X"] = ds->get_X();
            jd["Y"] = ds->get_Y();
            jd["angle"] = ds->get_angle();
            jd["format"] = ds->get_format();
            jd["target_net"] = compute_net_key_from_port(ds->port_at(0));
            j_scopes.push_back(std::move(jd));
            continue;
        }

        json je;
        je["class_id"] = cid;
        je["name"] = elem->get_name();
        if (!elem->get_src_location().empty())
            je["src"] = elem->get_src_location();
        const std::string out_w = first_out_wire_name(elem.get());
        if (!out_w.empty())
            je["out_wire"] = out_w;
        je["X"] = elem->get_X();
        je["Y"] = elem->get_Y();
        je["angle"] = elem->get_angle();
        if (cid == "model_nest:" && static_cast<model_nest *>(elem.get())->is_scale_2x())
            je["scale_2x"] = true;
        j_elems.push_back(std::move(je));

        /* Record wire colors by net key. */
        for (unsigned int pi = 0;; ++pi) {
            port *p = elem->port_at(pi);
            if (!p)
                break;
            for (const auto &w : p->get_wires()) {
                if (!w || !seen_wires.insert(w.get()).second)
                    continue;
                const std::string nk = compute_net_key_from_wire(w.get());
                if (!nk.empty())
                    j_wire_colors[nk] = w->get_color();
            }
        }
    }

    root["elements"] = std::move(j_elems);
    root["wire_colors"] = std::move(j_wire_colors);
    root["area_labels"] = std::move(j_labels);
    root["datascopes"] = std::move(j_scopes);

    try {
        std::filesystem::create_directories(path.parent_path());
        const auto tmp_path = path.string() + ".tmp";
        {
            std::ofstream out(tmp_path);
            if (!out)
                return false;
            out << root.dump(2) << '\n';
        }
        std::filesystem::rename(tmp_path, path);
    } catch (const std::exception &e) {
        logger::get().error("Failed to save layout overlay '{}': {}", path.string(), e.what());
        return false;
    }

    target_scene->clear_layout_dirty();
    logger::get().info("Saved layout overlay for '{}' -> {}",
                       target_scene->get_layout_module_name(), path.string());
    return true;
}

bool layout_overlay::apply_file_to_scene(class scene *target_scene, const std::filesystem::path &path)
{
    if (!target_scene || !std::filesystem::exists(path))
        return false;

    json root;
    try {
        std::ifstream in(path);
        if (!in)
            return false;
        in >> root;
    } catch (const std::exception &e) {
        logger::get().warn("Ignoring unreadable layout overlay '{}': {}", path.string(), e.what());
        return false;
    }

    if (!root.is_object())
        return false;

    /* 1. Build 2-tier lookup tables for saved element positions/rotations. */
    struct saved_elem {
        float X = 0.0f;
        float Y = 0.0f;
        int angle = 0;
        bool scale_2x = false;
        bool used = false;
    };
    std::vector<saved_elem> saved_list;
    std::unordered_map<std::string, size_t> exact_map;    /* class_id + "|" + name */
    std::unordered_map<std::string, size_t> fallback_map; /* class_id + "|" + src + "|" + out_wire */

    if (root.contains("elements") && root["elements"].is_array()) {
        saved_list.reserve(root["elements"].size());
        for (const auto &je : root["elements"]) {
            if (!je.is_object())
                continue;
            const std::string cid = je.value("class_id", "");
            const std::string nm = je.value("name", "");
            if (cid.empty())
                continue;
            saved_elem se;
            se.X = je.value("X", 0.0f);
            se.Y = je.value("Y", 0.0f);
            se.angle = ((je.value("angle", 0) % 360) + 360) % 360;
            se.scale_2x = je.value("scale_2x", false);
            const size_t idx = saved_list.size();
            saved_list.push_back(se);

            if (!nm.empty())
                exact_map.emplace(cid + "|" + nm, idx);

            const std::string src = je.value("src", "");
            const std::string out_w = je.value("out_wire", "");
            if (!src.empty() || !out_w.empty())
                fallback_map.emplace(cid + "|" + src + "|" + out_w, idx);
        }
    }

    std::unordered_set<const element *> matched_elems;
    size_t matched_count = 0;

    auto apply_pose = [&](element *e, const saved_elem &se) {
        if (e->class_id() == "model_nest:")
            static_cast<model_nest *>(e)->set_scale_2x(se.scale_2x);
        const int target_angle = (se.angle / 90) * 90;
        for (int tries = 0; tries < 4 && (e->get_angle() % 360) != target_angle; ++tries)
            e->rotate_ports();
        e->place(std::max(1.0f, std::floor(se.X)), std::max(1.0f, std::floor(se.Y)));
        e->reseat();
        target_scene->sizeX = std::max(target_scene->sizeX, e->get_X() + e->get_width() + 10.0f);
        target_scene->sizeY = std::max(target_scene->sizeY, e->get_Y() + e->get_height() + 10.0f);
    };

    /* Pass 1: exact (class_id, name) match. */
    for (auto &elem : target_scene->elements) {
        if (!elem || elem->is_background() || elem->class_id() == "connector:")
            continue;
        const std::string key = elem->class_id() + "|" + elem->get_name();
        auto it = exact_map.find(key);
        if (it != exact_map.end() && !saved_list[it->second].used) {
            saved_list[it->second].used = true;
            apply_pose(elem.get(), saved_list[it->second]);
            matched_elems.insert(elem.get());
            matched_count++;
        }
    }

    /* Pass 2: fallback (class_id, src, out_wire) match for renumbered components. */
    for (auto &elem : target_scene->elements) {
        if (!elem || elem->is_background() || elem->class_id() == "connector:" ||
            matched_elems.count(elem.get())) {
            continue;
        }
        const std::string src = elem->get_src_location();
        const std::string out_w = first_out_wire_name(elem.get());
        if (src.empty() && out_w.empty())
            continue;
        const std::string fkey = elem->class_id() + "|" + src + "|" + out_w;
        auto it = fallback_map.find(fkey);
        if (it != fallback_map.end() && !saved_list[it->second].used) {
            saved_list[it->second].used = true;
            apply_pose(elem.get(), saved_list[it->second]);
            matched_elems.insert(elem.get());
            matched_count++;
        }
    }

    /* Resolve collisions for any newly added (unmatched) elements against matched ones. */
    if (matched_count > 0 && matched_count < target_scene->elements.size()) {
        struct obox {
            int x1, y1, x2, y2;
        };
        std::vector<obox> placed_boxes;
        placed_boxes.reserve(target_scene->elements.size());
        for (const auto &elem : target_scene->elements) {
            if (!elem || elem->is_background() || elem->class_id() == "connector:")
                continue;
            if (!matched_elems.count(elem.get()))
                continue;
            const int ox = static_cast<int>(std::floor(elem->get_X()));
            const int oy = static_cast<int>(std::floor(elem->get_Y()));
            const int ow = std::max(1, static_cast<int>(std::ceil(elem->get_width())));
            const int oh = std::max(1, static_cast<int>(std::ceil(elem->get_height())));
            placed_boxes.push_back({ox - 1, oy - 1, ox + ow + 1, oy + oh + 1});
        }

        auto overlaps_placed = [&](int x, int y, int w, int h) -> bool {
            for (const auto &b : placed_boxes) {
                if (x <= b.x2 && x + w >= b.x1 && y <= b.y2 && y + h >= b.y1)
                    return true;
            }
            return false;
        };

        for (auto &elem : target_scene->elements) {
            if (!elem || elem->is_background() || elem->class_id() == "connector:" ||
                matched_elems.count(elem.get())) {
                continue;
            }
            int ex = static_cast<int>(std::floor(elem->get_X()));
            int ey = static_cast<int>(std::floor(elem->get_Y()));
            const int ew = std::max(1, static_cast<int>(std::ceil(elem->get_width())));
            const int eh = std::max(1, static_cast<int>(std::ceil(elem->get_height())));
            if (overlaps_placed(ex, ey, ew, eh)) {
                bool found = false;
                for (int r = 1; r <= 40 && !found; ++r) {
                    for (int dx = -r; dx <= r && !found; ++dx) {
                        const int rem = r - std::abs(dx);
                        const int dys[2] = {-rem, rem};
                        for (int oi = 0; oi < (rem == 0 ? 1 : 2); ++oi) {
                            const int cx = ex + dx;
                            const int cy = ey + dys[oi];
                            if (cx < 2 || cy < 2)
                                continue;
                            if (!overlaps_placed(cx, cy, ew, eh)) {
                                ex = cx;
                                ey = cy;
                                found = true;
                                break;
                            }
                        }
                    }
                }
                elem->place(ex, ey);
                elem->reseat();
            }
            placed_boxes.push_back({ex - 1, ey - 1, ex + ew + 1, ey + eh + 1});
            target_scene->sizeX = std::max(target_scene->sizeX, static_cast<float>(ex + ew + 10));
            target_scene->sizeY = std::max(target_scene->sizeY, static_cast<float>(ey + eh + 10));
        }
    }

    /* 2. Apply saved wire colors by net key. */
    if (root.contains("wire_colors") && root["wire_colors"].is_object()) {
        const auto &wc = root["wire_colors"];
        std::unordered_set<const wire *> visited;
        for (auto &elem : target_scene->elements) {
            if (!elem)
                continue;
            for (unsigned int pi = 0;; ++pi) {
                port *p = elem->port_at(pi);
                if (!p)
                    break;
                for (const auto &w : p->get_wires()) {
                    if (!w || !visited.insert(w.get()).second)
                        continue;
                    const std::string nk = compute_net_key_from_wire(w.get());
                    if (!nk.empty() && wc.contains(nk) && wc[nk].is_number_integer()) {
                        const int c = std::clamp(wc[nk].get<int>(), 0, 7);
                        w->push_wire_color(c);
                    }
                }
            }
        }
    }

    /* 3. Restore human-added Area Labels (model_label). */
    if (root.contains("area_labels") && root["area_labels"].is_array()) {
        for (const auto &jl : root["area_labels"]) {
            if (!jl.is_object())
                continue;
            const float lx = std::max(0.0f, jl.value("X", 4.0f));
            const float ly = std::max(0.0f, jl.value("Y", 4.0f));
            auto lbl = std::make_unique<model_label>(lx, ly);
            lbl->update_name(jl.value("name", "Area"));
            lbl->set_size(std::clamp(jl.value("sizeX", 8), 2, 200),
                          std::clamp(jl.value("sizeY", 6), 2, 200));
            lbl->set_color(std::clamp(jl.value("color", 0), 0, 16));
            lbl->set_bottomlabel(jl.value("bottomlabel", true));
            lbl->reseat();
            target_scene->add_element(std::move(lbl));
        }
    }

    /* 4. Restore human-added Data Scopes (model_datascope) and tap them onto their target net. */
    if (root.contains("datascopes") && root["datascopes"].is_array()) {
        /* Map net_key -> a representative wire on that net. */
        std::unordered_map<std::string, std::shared_ptr<wire>> net_key_to_wire;
        std::unordered_set<const wire *> seen;
        for (auto &elem : target_scene->elements) {
            if (!elem)
                continue;
            for (unsigned int pi = 0;; ++pi) {
                port *p = elem->port_at(pi);
                if (!p)
                    break;
                for (const auto &w : p->get_wires()) {
                    if (!w || !seen.insert(w.get()).second)
                        continue;
                    const std::string nk = compute_net_key_from_wire(w.get());
                    if (!nk.empty() && !net_key_to_wire.count(nk))
                        net_key_to_wire.emplace(nk, w);
                }
            }
        }

        for (const auto &jd : root["datascopes"]) {
            if (!jd.is_object())
                continue;
            const float dx = std::max(1.0f, std::floor(jd.value("X", 4.0f)));
            const float dy = std::max(1.0f, std::floor(jd.value("Y", 4.0f)));
            auto ds = std::make_unique<model_datascope>(dx, dy);
            ds->update_name(jd.value("name", ""));
            ds->set_format(std::clamp(jd.value("format", 3), 0, 3));
            const int target_angle = ((((jd.value("angle", 0) % 360) + 360) % 360) / 90) * 90;
            for (int tries = 0; tries < 4 && (ds->get_angle() % 360) != target_angle; ++tries)
                ds->rotate_ports();
            ds->reseat();
            target_scene->sizeX = std::max(target_scene->sizeX, ds->get_X() + ds->get_width() + 10.0f);
            target_scene->sizeY = std::max(target_scene->sizeY, ds->get_Y() + ds->get_height() + 10.0f);

            model_datascope *ds_raw = ds.get();
            target_scene->add_element(std::move(ds));

            const std::string target_net = jd.value("target_net", "");
            auto wit = net_key_to_wire.find(target_net);
            if (wit == net_key_to_wire.end() || !wit->second || !ds_raw->port_at(0))
                continue;

            std::shared_ptr<wire> tw = wit->second;
            const int w_width = std::max(1, tw->get_width());
            const int w_color = tw->get_color();

            /* Find an existing connector on tw, or insert one to split tw cleanly. */
            port *tap_port = nullptr;
            for (port *ep : tw->get_ports()) {
                if (ep && ep->parent && ep->parent->class_id() == "connector:") {
                    tap_port = ep;
                    break;
                }
            }
            if (!tap_port) {
                const auto &eps = tw->get_ports();
                port *p0 = (eps.size() >= 1) ? eps[0] : nullptr;
                port *p1 = (eps.size() >= 2) ? eps[1] : nullptr;
                if (p0 && p1) {
                    const float mx = std::round((p0->screenX + p1->screenX) * 0.5f);
                    const float my = std::round((p0->screenY + p1->screenY) * 0.5f);
                    auto conn = std::make_unique<connector>(mx, my);
                    connector *conn_raw = conn.get();
                    conn_raw->port_at(0)->set_width(w_width);
                    target_scene->add_element(std::move(conn));

                    auto make_copy = [&](port *a, port *b) {
                        auto nw = std::make_shared<wire>(
                            static_cast<int>(std::round(a->screenX)), static_cast<int>(std::round(a->screenY)),
                            static_cast<int>(std::round(b->screenX)), static_cast<int>(std::round(b->screenY)),
                            w_color);
                        nw->name = tw->name;
                        if (!tw->get_vcd_name().empty())
                            nw->set_vcd_name(tw->get_vcd_name());
                        if (!tw->get_vcd_valid_name().empty())
                            nw->set_vcd_valid_signal(tw->get_vcd_valid_name(), tw->is_vcd_valid_active_low());
                        if (tw->get_label_table())
                            nw->set_label_table(tw->get_label_table());
                        if (!tw->get_src_location().empty() || !tw->get_src_verilog().empty())
                            nw->set_src_info(tw->get_src_location(), tw->get_src_verilog());
                        nw->value = tw->value;
                        nw->set_width(w_width);
                        a->add_wire(nw);
                        b->add_wire(nw);
                        nw->push_wire_color(w_color);
                        return nw;
                    };

                    tw->remove();
                    auto nw1 = make_copy(p0, conn_raw->port_at(0));
                    make_copy(conn_raw->port_at(0), p1);
                    net_key_to_wire[target_net] = nw1;
                    tw = nw1;
                    tap_port = conn_raw->port_at(0);
                }
            }

            if (tap_port) {
                auto sw = std::make_shared<wire>(
                    static_cast<int>(std::round(tap_port->screenX)), static_cast<int>(std::round(tap_port->screenY)),
                    static_cast<int>(std::round(ds_raw->port_at(0)->screenX)), static_cast<int>(std::round(ds_raw->port_at(0)->screenY)),
                    w_color);
                sw->name = tw->name;
                if (!tw->get_vcd_name().empty())
                    sw->set_vcd_name(tw->get_vcd_name());
                if (!tw->get_vcd_valid_name().empty())
                    sw->set_vcd_valid_signal(tw->get_vcd_valid_name(), tw->is_vcd_valid_active_low());
                if (tw->get_label_table())
                    sw->set_label_table(tw->get_label_table());
                if (!tw->get_src_location().empty() || !tw->get_src_verilog().empty())
                    sw->set_src_info(tw->get_src_location(), tw->get_src_verilog());
                sw->value = tw->value;
                sw->set_width(w_width);
                tap_port->add_wire(sw);
                ds_raw->port_at(0)->add_wire(sw);
                sw->push_wire_color(w_color);
            }
        }
    }

    /* Reseat all wires to updated port coordinates and clear stale routes. */
    for (auto &elem : target_scene->elements) {
        if (!elem)
            continue;
        for (unsigned int pi = 0;; ++pi) {
            port *p = elem->port_at(pi);
            if (!p)
                break;
            for (const auto &w : p->get_wires()) {
                if (w) {
                    w->reseat();
                    w->clear_route();
                }
            }
        }
    }

    target_scene->clear_layout_dirty();
    logger::get().info("Applied layout overlay for '{}' ({} matched elements) from {}",
                       target_scene->get_layout_module_name(), matched_count, path.string());
    return true;
}

size_t layout_overlay::save_dirty_recursive(class scene *root_scene)
{
    if (!root_scene)
        return 0;

    size_t saved = 0;
    if (root_scene->is_layout_dirty() && !root_scene->get_layout_project_key().empty()) {
        const auto path = layout_file_for_module(root_scene->get_layout_project_key(),
                                                 root_scene->get_layout_module_name());
        if (save_scene_to_file(root_scene, path))
            saved++;
    }

    for (auto &elem : root_scene->elements) {
        if (elem && elem->class_id() == "model_nest:") {
            auto *nest = static_cast<model_nest *>(elem.get());
            if (scene *sub = nest->get_scene())
                saved += save_dirty_recursive(sub);
        }
    }
    return saved;
}

bool layout_overlay::apply_to_scene(class scene *target_scene, std::string_view project_key)
{
    if (!target_scene || project_key.empty())
        return false;
    const auto path = layout_file_for_module(project_key, target_scene->get_layout_module_name());
    return apply_file_to_scene(target_scene, path);
}

bool layout_overlay::reset_scene_layout(class scene *target_scene)
{
    if (!target_scene)
        return false;
    target_scene->clear_layout_dirty();
    if (target_scene->get_layout_project_key().empty())
        return false;
    const auto path = layout_file_for_module(target_scene->get_layout_project_key(),
                                             target_scene->get_layout_module_name());
    std::error_code ec;
    const bool removed = std::filesystem::remove(path, ec);
    if (removed)
        logger::get().info("Removed saved layout overlay '{}'", path.string());
    return removed;
}
