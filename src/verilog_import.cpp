/*
 * gridcad -- a tool to do digital circuit design with a GUI
 *
 * (C) Copyright 2025  -- Arjan van de Ven
 *
 * Licensed under the terms of the
 * GNU General Public LIcense version 3.0
 */

#include "verilog_import.h"
#include "placer.h"
#include "placement_optimizer.h"
#include "gridcad.h"
#include "scene.h"
#include "element.h"
#include "port.h"
#include "wire.h"
#include "connector.h"
#include "model_toggle.h"
#include "model_clock.h"
#include "model_output.h"
#include "model_const.h"
#include "model_and.h"
#include "model_or.h"
#include "model_nand.h"
#include "model_nor.h"
#include "model_xor.h"
#include "model_not.h"
#include "model_buffer.h"
#include "model_add.h"
#include "model_sub.h"
#include "model_mux.h"
#include "model_reg.h"
#include "model_eq.h"
#include "model_ne.h"
#include "model_gt.h"
#include "model_ge.h"
#include "model_mem.h"
#include "model_slice.h"
#include "model_concat.h"
#include "model_split.h"
#include "model_truth.h"
#include "model_nest.h"

#include <algorithm>
#include <charconv>
#include <filesystem>
#include <format>
#include <fstream>
#include <unordered_map>

using json = nlohmann::json;

namespace {

/* Parse an untrusted integer/hex/binary value from a JSON field safely. */
uint64_t parse_u64_json(const json &val, uint64_t fallback = 0)
{
    if (val.is_number_unsigned())
        return val.get<uint64_t>();
    if (val.is_number_integer()) {
        int64_t v = val.get<int64_t>();
        return static_cast<uint64_t>(v);
    }
    if (!val.is_string())
        return fallback;

    std::string s = val.get<std::string>();
    if (s.empty())
        return fallback;

    int base = 10;
    std::string_view sv = s;
    if (sv.size() >= 2 && sv[0] == '0' && (sv[1] == 'x' || sv[1] == 'X')) {
        base = 16;
        sv.remove_prefix(2);
    } else if (sv.size() >= 2 && sv[0] == '0' && (sv[1] == 'b' || sv[1] == 'B')) {
        base = 2;
        sv.remove_prefix(2);
    }
    if (sv.empty())
        return fallback;

    uint64_t parsed = 0;
    auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), parsed, base);
    if (ec != std::errc{} || ptr != sv.data() + sv.size())
        return fallback;
    return parsed;
}

int parse_width_json(const json &obj, std::string_view key, int fallback = 1)
{
    if (!obj.is_object() || !obj.contains(key))
        return std::clamp(fallback, 1, 64);
    const auto &v = obj[std::string(key)];
    int w = fallback;
    if (v.is_number_integer())
        w = v.get<int>();
    else if (v.is_string())
        w = static_cast<int>(parse_u64_json(v, static_cast<uint64_t>(fallback)));
    return std::clamp(w, 1, 64);
}

std::string make_endpoint_key(std::string_view kind, std::string_view id, std::string_view pin)
{
    std::string key;
    key.reserve(kind.size() + id.size() + pin.size() + 2);
    key.append(kind);
    key.push_back(':');
    key.append(id);
    key.push_back(':');
    key.append(pin);
    return key;
}

/* Build a stub scene for a black-box submodule or unsupported cell so external pins connect. */
std::unique_ptr<scene> build_stub_scene(const std::string &mod_name, const json &pins_obj)
{
    auto sub = std::make_unique<scene>(mod_name);
    int in_y = 4;
    int out_y = 4;

    if (pins_obj.is_object()) {
        for (auto it = pins_obj.begin(); it != pins_obj.end(); ++it) {
            const std::string pin_name = it.key();
            const json &pinfo = it.value();
            if (!pinfo.is_object())
                continue;
            const std::string dir = pinfo.value("direction", "input");
            const int width = parse_width_json(pinfo, "width", 1);

            if (dir == "output") {
                auto out = std::make_unique<model_output>(20, out_y);
                out->update_name(pin_name);
                out->set_width(width);
                if (out->port_at(0))
                    out->port_at(0)->update_name(pin_name);
                out->reseat();
                sub->add_element(std::move(out));
                out_y += 5;
            } else {
                auto in = std::make_unique<model_toggle>(4, in_y);
                in->update_name(pin_name);
                in->set_width(width);
                if (in->port_at(0))
                    in->port_at(0)->update_name(pin_name);
                in->reseat();
                sub->add_element(std::move(in));
                in_y += 5;
            }
        }
    }
    return sub;
}

using LabelTableMap = std::map<std::string, std::shared_ptr<const std::map<uint64_t, std::string>>>;

std::unique_ptr<scene> import_module_recursive(
    const json &modules_obj,
    const std::string &module_name,
    const std::string &instance_path,
    const LabelTableMap &label_tables,
    const verilog_importer::options &opts,
    const std::filesystem::path &base_dir,
    bool is_top,
    int depth,
    verilog_importer::stats &stats,
    std::string &error_out)
{
    if (depth > opts.max_recursion_depth) {
        error_out = std::format("Maximum submodule recursion depth ({}) exceeded at '{}'",
                                opts.max_recursion_depth, module_name);
        return nullptr;
    }

    if (!modules_obj.is_object() || !modules_obj.contains(module_name) ||
        !modules_obj[module_name].is_object()) {
        error_out = std::format("Module '{}' not found in JSON", module_name);
        return nullptr;
    }

    const json &mod = modules_obj[module_name];
    auto result_scene = std::make_unique<scene>(module_name);

    /* Maps "kind:id:pin" -> gridcad port* for wiring. */
    std::unordered_map<std::string, port *> port_map;

    /* 1. Module ports (sorted by declaration order when provided). */
    if (mod.contains("ports") && mod["ports"].is_array()) {
        std::vector<const json *> sorted_ports;
        sorted_ports.reserve(mod["ports"].size());
        for (const auto &p : mod["ports"]) {
            if (p.is_object())
                sorted_ports.push_back(&p);
        }
        std::stable_sort(sorted_ports.begin(), sorted_ports.end(),
                         [](const json *a, const json *b) {
                             return a->value("order", 0) < b->value("order", 0);
                         });

        for (const json *p : sorted_ports) {
            const std::string pname = p->value("name", "");
            const std::string dir = p->value("direction", "input");
            const int width = parse_width_json(*p, "width", 1);
            if (pname.empty())
                continue;

            if (dir == "input") {
                if (is_top && opts.use_clock_element && pname == "clk" && width == 1) {
                    auto clk = std::make_unique<model_clock>(0, 0);
                    clk->update_name(pname);
                    if (clk->port_at(0)) {
                        clk->port_at(0)->update_name(pname);
                        port_map[make_endpoint_key("port", pname, pname)] = clk->port_at(0);
                    }
                    result_scene->add_element(std::move(clk));
                } else {
                    auto tog = std::make_unique<model_toggle>(0, 0);
                    tog->update_name(pname);
                    tog->set_width(width);
                    if (tog->port_at(0)) {
                        tog->port_at(0)->update_name(pname);
                        port_map[make_endpoint_key("port", pname, pname)] = tog->port_at(0);
                    }
                    result_scene->add_element(std::move(tog));
                }
                stats.ports++;
            } else if (dir == "output") {
                auto out = std::make_unique<model_output>(0, 0);
                out->update_name(pname);
                out->set_width(width);
                if (out->port_at(0)) {
                    out->port_at(0)->update_name(pname);
                    port_map[make_endpoint_key("port", pname, pname)] = out->port_at(0);
                }
                result_scene->add_element(std::move(out));
                stats.ports++;
            } else {
                stats.warnings.push_back(std::format("Port '{}' in '{}' has unsupported direction '{}'",
                                                     pname, module_name, dir));
                auto tog = std::make_unique<model_toggle>(0, 0);
                tog->update_name(pname);
                tog->set_width(width);
                if (tog->port_at(0)) {
                    tog->port_at(0)->update_name(pname);
                    port_map[make_endpoint_key("port", pname, pname)] = tog->port_at(0);
                }
                result_scene->add_element(std::move(tog));
                stats.ports++;
            }
        }
    }

    /* 2. Constants. */
    if (mod.contains("constants") && mod["constants"].is_array()) {
        for (const auto &c : mod["constants"]) {
            if (!c.is_object())
                continue;
            const std::string cid = c.value("id", "");
            if (cid.empty())
                continue;
            const int width = parse_width_json(c, "width", 1);
            const uint64_t val = c.contains("value") ? parse_u64_json(c["value"], 0) : 0;

            auto cel = std::make_unique<model_const>(0, 0, width, val);
            cel->update_name(cid);
            if (cel->port_at(0))
                port_map[make_endpoint_key("constant", cid, "out")] = cel->port_at(0);
            result_scene->add_element(std::move(cel));
            stats.constants++;
        }
    }

    /* 3. Components. */
    if (mod.contains("components") && mod["components"].is_array()) {
        for (const auto &comp : mod["components"]) {
            if (!comp.is_object())
                continue;
            const std::string cid = comp.value("id", "");
            const std::string ctype = comp.value("type", "");
            const std::string inst = comp.value("instance_name", "");
            const bool is_sub = comp.value("is_submodule", false);
            const json params = comp.contains("parameters") && comp["parameters"].is_object()
                                    ? comp["parameters"]
                                    : json::object();
            if (cid.empty() || ctype.empty())
                continue;

            auto register_pin = [&](std::string_view pin_name, port *p) {
                if (p)
                    port_map[make_endpoint_key("component", cid, pin_name)] = p;
            };

            if (ctype == "AND2" || ctype == "OR2" || ctype == "NAND2" ||
                ctype == "NOR2" || ctype == "XOR2" ||
                ctype == "gridcad_and" || ctype == "gridcad_or" ||
                ctype == "gridcad_nand" || ctype == "gridcad_nor" ||
                ctype == "gridcad_xor") {
                const int width = parse_width_json(params, "WIDTH", 1);
                std::unique_ptr<model_logic2> gate;
                if (ctype == "AND2" || ctype == "gridcad_and")
                    gate = std::make_unique<model_and>(0, 0);
                else if (ctype == "OR2" || ctype == "gridcad_or")
                    gate = std::make_unique<model_or>(0, 0);
                else if (ctype == "NAND2" || ctype == "gridcad_nand")
                    gate = std::make_unique<model_nand>(0, 0);
                else if (ctype == "NOR2" || ctype == "gridcad_nor")
                    gate = std::make_unique<model_nor>(0, 0);
                else
                    gate = std::make_unique<model_xor>(0, 0);

                if (width > 1)
                    gate->set_bus_width(width);
                gate->update_name(cid);
                const bool lower_pins = ctype.starts_with("gridcad_");
                register_pin(lower_pins ? "a" : "A", gate->port_at(0));
                register_pin(lower_pins ? "b" : "B", gate->port_at(1));
                register_pin(lower_pins ? "y" : "Y", gate->port_at(2));
                result_scene->add_element(std::move(gate));
                stats.components++;
            } else if (ctype == "INV" || ctype == "gridcad_not") {
                const int width = parse_width_json(params, "WIDTH", 1);
                auto inv = std::make_unique<model_not>(0, 0);
                if (width > 1)
                    inv->set_bus_width(width);
                inv->update_name(cid);
                const bool lower_pins = (ctype == "gridcad_not");
                register_pin(lower_pins ? "a" : "A", inv->port_at(0));
                register_pin(lower_pins ? "y" : "Y", inv->port_at(1));
                result_scene->add_element(std::move(inv));
                stats.components++;
            } else if (ctype == "BUF") {
                auto buf = std::make_unique<model_buffer>(0, 0);
                buf->update_name(cid);
                /* Tie tri-state enable high so BUF acts as an always-on 1-bit buffer. */
                if (buf->port_at(1)) {
                    buf->port_at(1)->value.type = VALUE_TYPE_BOOL;
                    buf->port_at(1)->value.boolval = true;
                    buf->port_at(1)->value.intval = 1;
                    buf->port_at(1)->value.valid = true;
                }
                register_pin("A", buf->port_at(0));
                register_pin("Y", buf->port_at(2));
                result_scene->add_element(std::move(buf));
                stats.components++;
            } else if (ctype == "gridcad_add") {
                const int width = parse_width_json(params, "WIDTH", 8);
                auto el = std::make_unique<model_add>(0, 0, width);
                el->update_name(cid);
                register_pin("a", el->port_at(0));
                register_pin("b", el->port_at(1));
                register_pin("y", el->port_at(2));
                result_scene->add_element(std::move(el));
                stats.components++;
            } else if (ctype == "gridcad_sub") {
                const int width = parse_width_json(params, "WIDTH", 8);
                auto el = std::make_unique<model_sub>(0, 0, width);
                el->update_name(cid);
                register_pin("a", el->port_at(0));
                register_pin("b", el->port_at(1));
                register_pin("y", el->port_at(2));
                result_scene->add_element(std::move(el));
                stats.components++;
            } else if (ctype == "gridcad_mux") {
                const int width = parse_width_json(params, "WIDTH", 8);
                auto el = std::make_unique<model_mux>(0, 0, width);
                el->update_name(cid);
                register_pin("a", el->port_at(0));
                register_pin("b", el->port_at(1));
                register_pin("s", el->port_at(2));
                register_pin("y", el->port_at(3));
                result_scene->add_element(std::move(el));
                stats.components++;
            } else if (ctype == "gridcad_reg") {
                const int width = parse_width_json(params, "WIDTH", 8);
                const uint64_t reset_val = params.contains("RESET_VAL")
                                               ? parse_u64_json(params["RESET_VAL"], 0)
                                               : 0;
                auto el = std::make_unique<model_reg>(0, 0, width, reset_val);
                el->update_name(cid);
                register_pin("d", el->port_at(0));
                register_pin("en", el->port_at(1));
                register_pin("clk", el->port_at(2));
                register_pin("rst", el->port_at(3));
                register_pin("q", el->port_at(4));
                result_scene->add_element(std::move(el));
                stats.components++;
            } else if (ctype == "gridcad_eq") {
                const int width = parse_width_json(params, "WIDTH", 8);
                auto el = std::make_unique<model_eq>(0, 0, width);
                el->update_name(cid);
                register_pin("a", el->port_at(0));
                register_pin("b", el->port_at(1));
                register_pin("y", el->port_at(2));
                result_scene->add_element(std::move(el));
                stats.components++;
            } else if (ctype == "gridcad_ne") {
                const int width = parse_width_json(params, "WIDTH", 8);
                auto el = std::make_unique<model_ne>(0, 0, width);
                el->update_name(cid);
                register_pin("a", el->port_at(0));
                register_pin("b", el->port_at(1));
                register_pin("y", el->port_at(2));
                result_scene->add_element(std::move(el));
                stats.components++;
            } else if (ctype == "gridcad_gt") {
                const int width = parse_width_json(params, "WIDTH", 8);
                auto el = std::make_unique<model_gt>(0, 0, width);
                el->update_name(cid);
                register_pin("a", el->port_at(0));
                register_pin("b", el->port_at(1));
                register_pin("y", el->port_at(2));
                result_scene->add_element(std::move(el));
                stats.components++;
            } else if (ctype == "gridcad_ge") {
                const int width = parse_width_json(params, "WIDTH", 8);
                auto el = std::make_unique<model_ge>(0, 0, width);
                el->update_name(cid);
                register_pin("a", el->port_at(0));
                register_pin("b", el->port_at(1));
                register_pin("y", el->port_at(2));
                result_scene->add_element(std::move(el));
                stats.components++;
            } else if (ctype == "gridcad_slice") {
                const int in_w = parse_width_json(params, "IN_WIDTH", 8);
                const int out_w = parse_width_json(params, "OUT_WIDTH", 1);
                const int offset = params.contains("OFFSET")
                                       ? static_cast<int>(parse_u64_json(params["OFFSET"], 0))
                                       : 0;
                auto el = std::make_unique<model_slice>(0, 0, in_w, out_w, offset);
                el->update_name(cid);
                register_pin("in", el->port_at(0));
                register_pin("out", el->port_at(1));
                result_scene->add_element(std::move(el));
                stats.components++;
            } else if (ctype == "gridcad_concat") {
                const int out_w = parse_width_json(params, "OUT_WIDTH", 2);
                const int num_in = std::clamp(
                    params.contains("INPUTS") ? static_cast<int>(parse_u64_json(params["INPUTS"], 2)) : 2,
                    1, 64);
                std::vector<int> in_widths;
                in_widths.reserve(static_cast<size_t>(num_in));
                const json pins = (comp.contains("pins") && comp["pins"].is_object())
                                      ? comp["pins"]
                                      : json::object();
                for (int k = 0; k < num_in; ++k) {
                    const std::string pkey = std::format("in{}", k);
                    int w = 1;
                    if (pins.contains(pkey) && pins[pkey].is_object())
                        w = parse_width_json(pins[pkey], "width", 1);
                    in_widths.push_back(w);
                }
                auto el = std::make_unique<model_concat>(0, 0, in_widths, out_w);
                el->update_name(cid);
                for (int k = 0; k < num_in; ++k)
                    register_pin(std::format("in{}", k), el->port_at(static_cast<unsigned int>(k)));
                register_pin("out", el->port_at(static_cast<unsigned int>(num_in)));
                result_scene->add_element(std::move(el));
                stats.components++;
            } else if (ctype == "gridcad_split") {
                const int in_w = parse_width_json(params, "IN_WIDTH", 8);
                const int num_out = std::clamp(
                    params.contains("OUTPUTS") ? static_cast<int>(parse_u64_json(params["OUTPUTS"], in_w)) : in_w,
                    1, 64);
                std::vector<int> offsets;
                if (params.contains("OFFSETS") && params["OFFSETS"].is_array()) {
                    for (const auto &v : params["OFFSETS"])
                        offsets.push_back(static_cast<int>(parse_u64_json(v, 0)));
                }
                std::vector<int> out_widths;
                out_widths.reserve(static_cast<size_t>(num_out));
                const json pins = (comp.contains("pins") && comp["pins"].is_object())
                                      ? comp["pins"]
                                      : json::object();
                for (int k = 0; k < num_out; ++k) {
                    const std::string pkey = std::format("out{}", k);
                    int w = 1;
                    if (pins.contains(pkey) && pins[pkey].is_object())
                        w = parse_width_json(pins[pkey], "width", 1);
                    out_widths.push_back(w);
                }
                auto el = std::make_unique<model_split>(0, 0, in_w, num_out, offsets, out_widths);
                el->update_name(cid);
                register_pin("in", el->port_at(0));
                for (int k = 0; k < num_out; ++k)
                    register_pin(std::format("out{}", k), el->port_at(static_cast<unsigned int>(1 + k)));
                result_scene->add_element(std::move(el));
                stats.components++;
            } else if (ctype == "gridcad_lut") {
                const int width = std::clamp(parse_width_json(params, "WIDTH", 2), 1, 8);
                std::vector<std::string> lut_outputs;
                if (params.contains("LUT")) {
                    if (params["LUT"].is_array()) {
                        for (const auto &entry : params["LUT"]) {
                            if (entry.is_string())
                                lut_outputs.push_back(entry.get<std::string>());
                        }
                    } else if (params["LUT"].is_string()) {
                        lut_outputs.push_back(params["LUT"].get<std::string>());
                    }
                }
                if (lut_outputs.empty())
                    lut_outputs.emplace_back(size_t{1} << width, '0');

                auto el = std::make_unique<model_truth>(0, 0);
                el->update_name(cid);
                el->configure_lut(width, lut_outputs);
                for (int k = 0; k < width; ++k)
                    register_pin(std::format("A{}", k), el->port_at(static_cast<unsigned int>(k)));
                if (lut_outputs.size() == 1) {
                    register_pin("Y", el->port_at(static_cast<unsigned int>(width)));
                    register_pin("Y0", el->port_at(static_cast<unsigned int>(width)));
                } else {
                    for (size_t o = 0; o < lut_outputs.size(); ++o) {
                        register_pin(std::format("Y{}", o),
                                     el->port_at(static_cast<unsigned int>(width + o)));
                    }
                }
                result_scene->add_element(std::move(el));
                stats.components++;
            } else if (ctype == "gridcad_mem") {
                const int abits = parse_width_json(params, "ABITS", 4);
                const int dwidth = parse_width_json(params, "WIDTH", 8);
                const int msize = params.contains("SIZE")
                                      ? static_cast<int>(parse_u64_json(params["SIZE"], 16))
                                      : (1 << std::clamp(abits, 1, 16));
                const bool has_wr = params.contains("WR_PORTS") &&
                                    parse_u64_json(params["WR_PORTS"], 0) > 0;
                const bool rd_sync = params.contains("RD_CLK_ENABLE") &&
                                     parse_u64_json(params["RD_CLK_ENABLE"], 0) != 0;
                const bool wr_sync = !params.contains("WR_CLK_ENABLE") ||
                                     parse_u64_json(params["WR_CLK_ENABLE"], 1) != 0;
                int wr_en_w = 1;
                if (comp.contains("pins") && comp["pins"].is_object() &&
                    comp["pins"].contains("WR_EN") && comp["pins"]["WR_EN"].is_object()) {
                    wr_en_w = parse_width_json(comp["pins"]["WR_EN"], "width", 1);
                }

                auto el = std::make_unique<model_mem>(0, 0, abits, dwidth, msize,
                                                      has_wr, rd_sync, wr_sync, wr_en_w);
                el->update_name(inst.empty() ? cid : inst);
                if (params.contains("INIT") && params["INIT"].is_string())
                    el->load_init_string(params["INIT"].get<std::string>());

                register_pin("RD_ADDR", el->port_at(0));
                register_pin("RD_EN", el->port_at(1));
                register_pin("RD_CLK", el->port_at(2));
                register_pin("RD_ARST", el->port_at(3));
                register_pin("RD_SRST", el->port_at(4));
                register_pin("RD_DATA", el->port_at(5));
                if (has_wr) {
                    register_pin("WR_ADDR", el->port_at(6));
                    register_pin("WR_DATA", el->port_at(7));
                    register_pin("WR_EN", el->port_at(8));
                    register_pin("WR_CLK", el->port_at(9));
                }
                result_scene->add_element(std::move(el));
                stats.components++;
            } else if (is_sub) {
                std::unique_ptr<scene> sub_scene;
                std::string sub_err;
                const std::string sub_inst_name = inst.empty() ? cid : inst;
                const std::string sub_path = instance_path.empty()
                                                 ? sub_inst_name
                                                 : (instance_path + "." + sub_inst_name);

                if (modules_obj.contains(ctype)) {
                    sub_scene = import_module_recursive(modules_obj, ctype, sub_path, label_tables,
                                                        opts, base_dir, false, depth + 1, stats, sub_err);
                } else if (opts.load_sibling_files && !base_dir.empty()) {
                    const std::filesystem::path sibling = base_dir / (ctype + ".json");
                    std::error_code ec;
                    if (std::filesystem::is_regular_file(sibling, ec)) {
                        std::ifstream sf(sibling);
                        if (sf) {
                            try {
                                json sj;
                                sf >> sj;
                                if (sj.is_object() && sj.contains("modules") &&
                                    sj["modules"].is_object() && sj["modules"].contains(ctype)) {
                                    sub_scene = import_module_recursive(
                                        sj["modules"], ctype, sub_path, label_tables, opts, base_dir,
                                        false, depth + 1, stats, sub_err);
                                }
                            } catch (const std::exception &e) {
                                stats.warnings.push_back(
                                    std::format("Failed to parse sibling '{}': {}",
                                                sibling.string(), e.what()));
                            }
                        }
                    }
                }

                if (!sub_scene) {
                    const json pins = comp.contains("pins") ? comp["pins"] : json::object();
                    sub_scene = build_stub_scene(ctype, pins);
                    stats.warnings.push_back(
                        std::format("Submodule '{}' (type '{}') imported as black-box stub",
                                    inst.empty() ? cid : inst, ctype));
                }

                auto nest = std::make_unique<model_nest>(0, 0);
                nest->update_name(inst.empty() ? cid : inst);
                nest->set_scene(std::move(sub_scene));

                for (unsigned int pi = 0;; ++pi) {
                    port *p = nest->port_at(pi);
                    if (!p)
                        break;
                    register_pin(p->name, p);
                }
                result_scene->add_element(std::move(nest));
                stats.submodules++;
            } else {
                stats.unsupported_components++;
                stats.warnings.push_back(
                    std::format("Unsupported component type '{}' (id '{}') in '{}' -> black-box stub",
                                ctype, cid, module_name));
                const json pins = comp.contains("pins") ? comp["pins"] : json::object();
                auto stub_scene = build_stub_scene(ctype, pins);
                auto nest = std::make_unique<model_nest>(0, 0);
                nest->update_name(std::format("{} ({})", ctype, cid));
                nest->set_scene(std::move(stub_scene));

                for (unsigned int pi = 0;; ++pi) {
                    port *p = nest->port_at(pi);
                    if (!p)
                        break;
                    register_pin(p->name, p);
                }
                result_scene->add_element(std::move(nest));
            }
        }
    }

    /* 4. Wires. */
    if (mod.contains("wires") && mod["wires"].is_array()) {
        for (const auto &wobj : mod["wires"]) {
            if (!wobj.is_object())
                continue;
            if (!wobj.contains("driver") || !wobj["driver"].is_object())
                continue;
            if (!wobj.contains("sinks") || !wobj["sinks"].is_array())
                continue;

            const std::string wname = wobj.value("name", "");
            const std::string vcd_local = wobj.value("vcd_signal", "");
            const std::string lbl_key = wobj.value("label_table", "");
            std::shared_ptr<const std::map<uint64_t, std::string>> wire_lbl_tbl;
            if (!lbl_key.empty()) {
                auto lit = label_tables.find(lbl_key);
                if (lit != label_tables.end())
                    wire_lbl_tbl = lit->second;
            }
            const std::string full_vcd_name =
                vcd_local.empty()
                    ? std::string("")
                    : (instance_path.empty() ? vcd_local : (instance_path + "." + vcd_local));
            if (wobj.contains("width") && wobj["width"].is_number_integer() &&
                wobj["width"].get<int>() == 0)
                continue;
            const int width = parse_width_json(wobj, "width", 1);

            const auto &drv = wobj["driver"];
            const std::string drv_key = make_endpoint_key(
                drv.value("kind", ""), drv.value("id", ""), drv.value("pin", ""));
            auto dit = port_map.find(drv_key);
            if (dit == port_map.end() || !dit->second) {
                stats.missing_endpoints++;
                stats.warnings.push_back(
                    std::format("Wire '{}' in '{}': missing driver '{}'",
                                wname, module_name, drv_key));
                continue;
            }
            port *drv_port = dit->second;

            /* Collect valid sink ports first so we know whether fanout needs a connector split. */
            std::vector<port *> sink_ports;
            sink_ports.reserve(wobj["sinks"].size());
            for (const auto &snk : wobj["sinks"]) {
                if (!snk.is_object())
                    continue;
                const std::string snk_key = make_endpoint_key(
                    snk.value("kind", ""), snk.value("id", ""), snk.value("pin", ""));
                auto sit = port_map.find(snk_key);
                if (sit == port_map.end() || !sit->second) {
                    stats.missing_endpoints++;
                    stats.warnings.push_back(
                        std::format("Wire '{}' in '{}': missing sink '{}'",
                                    wname, module_name, snk_key));
                    continue;
                }
                sink_ports.push_back(sit->second);
            }

            if (sink_ports.empty())
                continue;

            auto connect_ports = [&](port *a, port *b) {
                auto w = std::make_shared<wire>(
                    static_cast<int>(a->screenX), static_cast<int>(a->screenY),
                    static_cast<int>(b->screenX), static_cast<int>(b->screenY));
                if (!wname.empty())
                    w->name = wname;
                if (!full_vcd_name.empty())
                    w->set_vcd_name(full_vcd_name);
                if (wire_lbl_tbl)
                    w->set_label_table(wire_lbl_tbl);
                if (wname == "clk" || vcd_local == "clk") {
                    w->value.valid = true;
                    w->value.is_clock = true;
                }
                w->set_width(width);
                a->add_wire(w);
                b->add_wire(w);
                stats.wires++;
            };

            if (sink_ports.size() == 1) {
                connect_ports(drv_port, sink_ports[0]);
            } else {
                /* Multi-sink fanout: insert a native gridcad connector split junction. */
                auto conn = std::make_unique<connector>(0, 0);
                port *conn_port = conn->port_at(0);
                if (conn_port) {
                    conn_port->set_width(width);
                    connect_ports(drv_port, conn_port);
                    for (port *snk_port : sink_ports)
                        connect_ports(conn_port, snk_port);
                }
                result_scene->add_element(std::move(conn));
            }
        }
    }

    /* 5. Topological placement, automatic optimization rounds & net initialization. */
    if (opts.run_placer) {
        topological_placer::place(result_scene.get());
        if (opts.optimize_rounds > 0)
            placement_optimizer::optimize_rounds(result_scene.get(), opts.optimize_rounds);
    }

    result_scene->redo_nets();

    /* Propagate initial values from constants and registers into the netlist. */
    for (auto &elem : result_scene->elements) {
        const std::string cid = elem->class_id();
        if (cid == "model_const:" || cid == "model_reg:" || cid == "model_mem:")
            elem->calculate(DEFAULT_TTL);
    }
    run_queued_calculations();

    return result_scene;
}

} // namespace

verilog_importer::result verilog_importer::import_file(std::string_view json_path)
{
    return import_file(json_path, options{});
}

verilog_importer::result verilog_importer::import_file(std::string_view json_path, const options &opts)
{
    result res;
    std::filesystem::path p{std::string(json_path)};

    std::ifstream input(p);
    if (!input) {
        res.error = std::format("Could not open JSON file '{}'", p.string());
        return res;
    }

    json root;
    try {
        input >> root;
    } catch (const std::exception &e) {
        res.error = std::format("JSON parse error in '{}': {}", p.string(), e.what());
        return res;
    }

    return import_json(root, opts, p.parent_path());
}

verilog_importer::result verilog_importer::import_json(const json &root,
                                                       const std::filesystem::path &base_dir)
{
    return import_json(root, options{}, base_dir);
}

verilog_importer::result verilog_importer::import_json(const json &root,
                                                       const options &opts,
                                                       const std::filesystem::path &base_dir)
{
    result res;

    if (!root.is_object()) {
        res.error = "Top-level JSON value must be an object";
        return res;
    }
    if (!root.contains("modules") || !root["modules"].is_object()) {
        res.error = "Missing or invalid 'modules' object in JSON";
        return res;
    }

    res.top_module = root.value("top_module", "");
    if (res.top_module.empty()) {
        if (root["modules"].size() == 1)
            res.top_module = root["modules"].begin().key();
        else {
            res.error = "Missing 'top_module' in JSON";
            return res;
        }
    }

    /* Parse optional enum/state label tables for waveform/wire annotation. */
    LabelTableMap shared_label_tables;
    if (root.contains("label_tables") && root["label_tables"].is_object()) {
        for (auto it = root["label_tables"].begin(); it != root["label_tables"].end(); ++it) {
            if (!it.value().is_object())
                continue;
            auto &tbl = res.label_tables[it.key()];
            for (auto kv = it.value().begin(); kv != it.value().end(); ++kv) {
                if (kv.value().is_string()) {
                    uint64_t code = parse_u64_json(json(kv.key()), 0);
                    tbl[code] = kv.value().get<std::string>();
                }
            }
            shared_label_tables[it.key()] =
                std::make_shared<std::map<uint64_t, std::string>>(tbl);
        }
    }

    res.imported_scene = import_module_recursive(
        root["modules"], res.top_module, "", shared_label_tables, opts, base_dir, true, 0, res.stats, res.error);
    res.ok = (res.imported_scene != nullptr);
    return res;
}
