/*
 * gridcad -- a tool to do digital circuit design with a GUI
 *
 * (C) Copyright 2025  -- Arjan van de Ven
 *
 * Licensed under the terms of the
 * GNU General Public LIcense version 3.0
 */

#include "gridcad.h"
#include "scene.h"
#include "element.h"
#include "port.h"
#include "wire.h"
#include "wirepath.h"
#include "model_toggle.h"
#include "model_output.h"
#include "model_and.h"
#include "model_or.h"
#include "model_nand.h"
#include "model_nor.h"
#include "model_xor.h"
#include "model_not.h"
#include "model_const.h"
#include "model_add.h"
#include "model_sub.h"
#include "model_mux.h"
#include "model_pmux.h"
#include "model_reg.h"
#include "model_eq.h"
#include "model_ne.h"
#include "model_gt.h"
#include "model_ge.h"
#include "model_reduce.h"
#include "model_mem.h"
#include "model_slice.h"
#include "model_concat.h"
#include "model_split.h"
#include "model_truth.h"
#include "connector.h"
#include "verilog_import.h"
#include "vcd_session.h"
#include "timelinebar.h"
#include "placer.h"
#include "layout_overlay.h"
#include "model_label.h"
#include "model_nest.h"
#include "model_datascope.h"

#include <cassert>
#include <filesystem>
#include <iostream>
#include <memory>
#include <set>
#include <nlohmann/json.hpp>

static void connect_ports(class port *p1, class port *p2)
{
    auto w = std::make_shared<wire>(0, 0, 0, 0);
    w->add_port(p1);
    w->add_port(p2);
    p1->add_wire(w);
    p2->add_wire(w);
    w->reseat();
}

static struct value make_bool_val(bool b)
{
    struct value v = {};
    v.valid = true;
    v.type = VALUE_TYPE_BOOL;
    v.boolval = b;
    v.intval = b ? 1 : 0;
    return v;
}

static void test_factory(void)
{
    const char *known[] = {
        "model_and:", "model_or:", "model_nand:", "model_nor:", "model_xor:",
        "model_not:", "model_buffer:", "model_zero:", "model_one:",
        "model_toggle:", "model_output:", "model_dflipflop:", "model_deflipflop:",
        "model_memory:", "model_clock:", "model_label:", "model_truth:",
        "model_nest:", "sequencer:", "connector:",
        "model_const:", "model_add:", "model_sub:", "model_mux:",
        "model_reg:", "model_eq:", "model_ne:"
    };

    for (const char *cid : known) {
        auto elem = element_from_class_id(cid);
        assert(elem != nullptr);
        assert(elem->class_id() == cid);
    }

    assert(element_from_class_id("nonexistent_model:") == nullptr);
}

template <typename GateType>
static void verify_binary_gate(bool expected[4])
{
    scene s("test_gate");

    auto in_a = std::make_unique<model_toggle>(2, 2);
    auto in_b = std::make_unique<model_toggle>(2, 6);
    auto gate = std::make_unique<GateType>(10, 4);
    auto out  = std::make_unique<model_output>(18, 4);

    auto *a_ptr = in_a.get();
    auto *b_ptr = in_b.get();
    auto *g_ptr = gate.get();
    auto *o_ptr = out.get();

    s.add_element(std::move(in_a));
    s.add_element(std::move(in_b));
    s.add_element(std::move(gate));
    s.add_element(std::move(out));

    connect_ports(a_ptr->port_at(0), g_ptr->port_at(0));
    connect_ports(b_ptr->port_at(0), g_ptr->port_at(1));
    connect_ports(g_ptr->port_at(2), o_ptr->port_at(0));
    s.redo_nets();

    for (int i = 0; i < 4; ++i) {
        bool a = (i & 1) != 0;
        bool b = (i & 2) != 0;
        auto va = make_bool_val(a);
        auto vb = make_bool_val(b);
        a_ptr->update_value(&va, DEFAULT_TTL);
        b_ptr->update_value(&vb, DEFAULT_TTL);
        run_queued_calculations();

        struct value res = o_ptr->get_value();
        assert(res.valid);
        assert(res.boolval == expected[i]);
    }
}

static void test_logic_gates(void)
{
    // Index bit 0 = A, bit 1 = B: (0,0), (1,0), (0,1), (1,1)
    bool and_exp[4]  = {false, false, false, true};
    bool or_exp[4]   = {false, true,  true,  true};
    bool nand_exp[4] = {true,  true,  true,  false};
    bool nor_exp[4]  = {true,  false, false, false};
    bool xor_exp[4]  = {false, true,  true,  false};

    verify_binary_gate<model_and>(and_exp);
    verify_binary_gate<model_or>(or_exp);
    verify_binary_gate<model_nand>(nand_exp);
    verify_binary_gate<model_nor>(nor_exp);
    verify_binary_gate<model_xor>(xor_exp);

    // Inverter (NOT)
    scene s("test_not");
    auto in_a = std::make_unique<model_toggle>(2, 2);
    auto inv  = std::make_unique<model_not>(10, 2);
    auto out  = std::make_unique<model_output>(18, 2);

    auto *a_ptr = in_a.get();
    auto *i_ptr = inv.get();
    auto *o_ptr = out.get();

    s.add_element(std::move(in_a));
    s.add_element(std::move(inv));
    s.add_element(std::move(out));

    connect_ports(a_ptr->port_at(0), i_ptr->port_at(0));
    connect_ports(i_ptr->port_at(1), o_ptr->port_at(0));
    s.redo_nets();

    for (bool a : {false, true}) {
        auto va = make_bool_val(a);
        a_ptr->update_value(&va, DEFAULT_TTL);
        run_queued_calculations();
        struct value res = o_ptr->get_value();
        assert(res.valid);
        assert(res.boolval == !a);
    }
}

static void test_wirepath_routing(void)
{
    wiregrid grid(50, 33);
    for (int y = 20; y < 30; ++y)
        grid.block_point(20, y);

    auto points = grid.path_walk(15, 25, 25, 25);
    assert(points != nullptr);
    assert(points->size() >= 2);
    assert(points->front().X == 15 && points->front().Y == 25);
    assert(points->back().X == 25 && points->back().Y == 25);
}

static void test_scene_json_and_verilog(void)
{
    json saved;
    {
        scene s("roundtrip");
        auto in_a = std::make_unique<model_toggle>(2, 2);
        auto in_b = std::make_unique<model_toggle>(2, 6);
        auto gate = std::make_unique<model_and>(10, 4);
        auto out  = std::make_unique<model_output>(18, 4);

        auto *a_ptr = in_a.get();
        auto *b_ptr = in_b.get();
        auto *g_ptr = gate.get();
        auto *o_ptr = out.get();

        s.add_element(std::move(in_a));
        s.add_element(std::move(in_b));
        s.add_element(std::move(gate));
        s.add_element(std::move(out));

        connect_ports(a_ptr->port_at(0), g_ptr->port_at(0));
        connect_ports(b_ptr->port_at(0), g_ptr->port_at(1));
        connect_ports(g_ptr->port_at(2), o_ptr->port_at(0));
        s.redo_nets();

        auto v1 = make_bool_val(true);
        a_ptr->update_value(&v1, DEFAULT_TTL);
        b_ptr->update_value(&v1, DEFAULT_TTL);
        run_queued_calculations();

        s.to_json(saved);

        std::string verilog = s.get_verilog_main();
        assert(verilog.find("module ") != std::string::npos);
        assert(verilog.find("endmodule") != std::string::npos);
    }

    clear_wire_factory();
    scene loaded("roundtrip");
    loaded.from_json(saved);
    clear_wire_factory();

    assert(loaded.elements.size() == 4);
}

static struct value make_int_val(uint64_t x)
{
    struct value v = {};
    v.valid = true;
    v.type = VALUE_TYPE_INT;
    v.intval = x;
    v.boolval = (x != 0);
    return v;
}

static void test_parameterized_elements(void)
{
    // 1. model_const + model_add + model_sub + model_mux + model_eq + model_ne (16-bit)
    {
        scene s("param_alu");
        auto in_a  = std::make_unique<model_toggle>(1, 2);
        auto c_two = std::make_unique<model_const>(1, 6, 16, 2);
        auto sel   = std::make_unique<model_toggle>(1, 10);
        auto adder = std::make_unique<model_add>(8, 2, 16);
        auto sub   = std::make_unique<model_sub>(8, 6, 16);
        auto mux   = std::make_unique<model_mux>(14, 4, 16);
        auto eq    = std::make_unique<model_eq>(14, 10, 16);
        auto ne    = std::make_unique<model_ne>(14, 14, 16);
        auto out_m = std::make_unique<model_output>(22, 4);
        auto out_e = std::make_unique<model_output>(22, 10);
        auto out_n = std::make_unique<model_output>(22, 14);

        in_a->set_width(16);
        out_m->set_width(16);

        auto *a_p = in_a.get();
        auto *c_p = c_two.get();
        auto *s_p = sel.get();
        auto *add_p = adder.get();
        auto *sub_p = sub.get();
        auto *mux_p = mux.get();
        auto *eq_p  = eq.get();
        auto *ne_p  = ne.get();
        auto *om_p  = out_m.get();
        auto *oe_p  = out_e.get();
        auto *on_p  = out_n.get();

        s.add_element(std::move(in_a));
        s.add_element(std::move(c_two));
        s.add_element(std::move(sel));
        s.add_element(std::move(adder));
        s.add_element(std::move(sub));
        s.add_element(std::move(mux));
        s.add_element(std::move(eq));
        s.add_element(std::move(ne));
        s.add_element(std::move(out_m));
        s.add_element(std::move(out_e));
        s.add_element(std::move(out_n));

        connect_ports(a_p->port_at(0), add_p->port_at(0));
        connect_ports(c_p->port_at(0), add_p->port_at(1));
        connect_ports(a_p->port_at(0), sub_p->port_at(0));
        connect_ports(c_p->port_at(0), sub_p->port_at(1));
        connect_ports(add_p->port_at(2), mux_p->port_at(0));
        connect_ports(sub_p->port_at(2), mux_p->port_at(1));
        connect_ports(s_p->port_at(0), mux_p->port_at(2));
        connect_ports(mux_p->port_at(3), om_p->port_at(0));

        connect_ports(a_p->port_at(0), eq_p->port_at(0));
        connect_ports(c_p->port_at(0), eq_p->port_at(1));
        connect_ports(eq_p->port_at(2), oe_p->port_at(0));

        connect_ports(a_p->port_at(0), ne_p->port_at(0));
        connect_ports(c_p->port_at(0), ne_p->port_at(1));
        connect_ports(ne_p->port_at(2), on_p->port_at(0));

        s.redo_nets();
        c_p->calculate(DEFAULT_TTL);

        // a = 0x0200, s = 0 -> mux selects a + 2 = 0x0202
        auto v_a = make_int_val(0x0200);
        auto v_s0 = make_bool_val(false);
        a_p->update_value(&v_a, DEFAULT_TTL);
        s_p->update_value(&v_s0, DEFAULT_TTL);
        run_queued_calculations();
        assert(om_p->get_value().intval == 0x0202);
        assert(oe_p->get_value().boolval == false);
        assert(on_p->get_value().boolval == true);

        // s = 1 -> mux selects a - 2 = 0x01FE
        auto v_s1 = make_bool_val(true);
        s_p->update_value(&v_s1, DEFAULT_TTL);
        run_queued_calculations();
        assert(om_p->get_value().intval == 0x01FE);

        // a = 2 -> eq == true, ne == false, sub == 0
        auto v_a2 = make_int_val(2);
        a_p->update_value(&v_a2, DEFAULT_TTL);
        run_queued_calculations();
        assert(om_p->get_value().intval == 0);
        assert(oe_p->get_value().boolval == true);
        assert(on_p->get_value().boolval == false);

        // 16-bit wraparound: a = 0, sub 2 -> 0xFFFE
        auto v_a0 = make_int_val(0);
        a_p->update_value(&v_a0, DEFAULT_TTL);
        run_queued_calculations();
        assert(om_p->get_value().intval == 0xFFFE);
    }

    // 2. model_reg (16-bit with async reset + RESET_VAL + enable + posedge clk)
    {
        scene s("param_reg");
        auto in_d   = std::make_unique<model_toggle>(1, 2);
        auto in_en  = std::make_unique<model_toggle>(1, 6);
        auto in_clk = std::make_unique<model_toggle>(1, 10);
        auto in_rst = std::make_unique<model_toggle>(1, 14);
        auto reg    = std::make_unique<model_reg>(10, 6, 16, 0x0200);
        auto out_q  = std::make_unique<model_output>(18, 6);

        in_d->set_width(16);
        out_q->set_width(16);

        auto *d_p   = in_d.get();
        auto *en_p  = in_en.get();
        auto *clk_p = in_clk.get();
        auto *rst_p = in_rst.get();
        auto *reg_p = reg.get();
        auto *q_p   = out_q.get();

        s.add_element(std::move(in_d));
        s.add_element(std::move(in_en));
        s.add_element(std::move(in_clk));
        s.add_element(std::move(in_rst));
        s.add_element(std::move(reg));
        s.add_element(std::move(out_q));

        connect_ports(d_p->port_at(0),   reg_p->port_at(0));
        connect_ports(en_p->port_at(0),  reg_p->port_at(1));
        connect_ports(clk_p->port_at(0), reg_p->port_at(2));
        connect_ports(rst_p->port_at(0), reg_p->port_at(3));
        connect_ports(reg_p->port_at(4), q_p->port_at(0));
        s.redo_nets();

        auto v_lo = make_bool_val(false);
        auto v_hi = make_bool_val(true);
        auto v_d1 = make_int_val(0x1234);

        // Assert reset -> Q = 0x0200
        rst_p->update_value(&v_hi, DEFAULT_TTL);
        clk_p->update_value(&v_lo, DEFAULT_TTL);
        en_p->update_value(&v_lo, DEFAULT_TTL);
        d_p->update_value(&v_d1, DEFAULT_TTL);
        run_queued_calculations();
        assert(q_p->get_value().intval == 0x0200);

        // Deassert reset, clock edge with en=0 -> Q stays 0x0200
        rst_p->update_value(&v_lo, DEFAULT_TTL);
        clk_p->update_value(&v_hi, DEFAULT_TTL);
        run_queued_calculations();
        assert(q_p->get_value().intval == 0x0200);

        // Falling edge, then rising edge with en=1 -> Q latches 0x1234
        clk_p->update_value(&v_lo, DEFAULT_TTL);
        en_p->update_value(&v_hi, DEFAULT_TTL);
        run_queued_calculations();
        clk_p->update_value(&v_hi, DEFAULT_TTL);
        run_queued_calculations();
        assert(q_p->get_value().intval == 0x1234);

        // Async reset brings Q back to 0x0200
        rst_p->update_value(&v_hi, DEFAULT_TTL);
        run_queued_calculations();
        assert(q_p->get_value().intval == 0x0200);
    }
}

static void test_verilog_import(void)
{
    // 1. Inline Turbostar JSON fixture with fanout (connector split), mux, add, const
    {
        const json fixture = {
            {"top_module", "mini_alu"},
            {"modules", {
                {"mini_alu", {
                    {"ports", {
                        {{"name", "a"}, {"direction", "input"}, {"width", 8}, {"order", 0}},
                        {{"name", "sel"}, {"direction", "input"}, {"width", 1}, {"order", 1}},
                        {{"name", "y"}, {"direction", "output"}, {"width", 8}, {"order", 2}}
                    }},
                    {"constants", {
                        {{"id", "c0"}, {"value", "0x05"}, {"width", 8}}
                    }},
                    {"components", {
                        {{"id", "add0"}, {"type", "gridcad_add"}, {"is_submodule", false},
                         {"parameters", {{"WIDTH", 8}}}},
                        {{"id", "sub0"}, {"type", "gridcad_sub"}, {"is_submodule", false},
                         {"parameters", {{"WIDTH", 8}}}},
                        {{"id", "mux0"}, {"type", "gridcad_mux"}, {"is_submodule", false},
                         {"parameters", {{"WIDTH", 8}}}}
                    }},
                    {"wires", {
                        {{"name", "a"}, {"width", 8},
                         {"driver", {{"kind", "port"}, {"id", "a"}, {"pin", "a"}}},
                         {"sinks", {
                             {{"kind", "component"}, {"id", "add0"}, {"pin", "a"}},
                             {{"kind", "component"}, {"id", "sub0"}, {"pin", "a"}}
                         }}},
                        {{"name", "k5"}, {"width", 8},
                         {"driver", {{"kind", "constant"}, {"id", "c0"}, {"pin", "out"}}},
                         {"sinks", {
                             {{"kind", "component"}, {"id", "add0"}, {"pin", "b"}},
                             {{"kind", "component"}, {"id", "sub0"}, {"pin", "b"}}
                         }}},
                        {{"name", "sum"}, {"width", 8},
                         {"driver", {{"kind", "component"}, {"id", "add0"}, {"pin", "y"}}},
                         {"sinks", {{{"kind", "component"}, {"id", "mux0"}, {"pin", "a"}}}}},
                        {{"name", "diff"}, {"width", 8},
                         {"driver", {{"kind", "component"}, {"id", "sub0"}, {"pin", "y"}}},
                         {"sinks", {{{"kind", "component"}, {"id", "mux0"}, {"pin", "b"}}}}},
                        {{"name", "sel"}, {"width", 1},
                         {"driver", {{"kind", "port"}, {"id", "sel"}, {"pin", "sel"}}},
                         {"sinks", {{{"kind", "component"}, {"id", "mux0"}, {"pin", "s"}}}}},
                        {{"name", "y"}, {"width", 8},
                         {"driver", {{"kind", "component"}, {"id", "mux0"}, {"pin", "y"}}},
                         {"sinks", {{{"kind", "port"}, {"id", "y"}, {"pin", "y"}}}}}
                    }}
                }}
            }}
        };

        verilog_importer::options opts;
        opts.use_clock_element = false;
        auto res = verilog_importer::import_json(fixture, opts);
        assert(res.ok);
        assert(res.imported_scene != nullptr);
        assert(res.stats.ports == 3);
        assert(res.stats.constants == 1);
        assert(res.stats.components == 3);
        assert(res.stats.missing_endpoints == 0);
        assert(res.stats.warnings.empty());

        // Find inputs/output and verify connector splits + simulation
        model_toggle *a_tog = nullptr;
        model_toggle *sel_tog = nullptr;
        model_output *y_out = nullptr;
        int connector_count = 0;
        for (auto &e : res.imported_scene->elements) {
            if (e->class_id() == "connector:")
                connector_count++;
            else if (e->class_id() == "model_toggle:" && e->get_name() == "a")
                a_tog = static_cast<model_toggle *>(e.get());
            else if (e->class_id() == "model_toggle:" && e->get_name() == "sel")
                sel_tog = static_cast<model_toggle *>(e.get());
            else if (e->class_id() == "model_output:" && e->get_name() == "y")
                y_out = static_cast<model_output *>(e.get());
        }
        assert(connector_count == 2); // 'a' and 'k5' both fan out to 2 sinks
        assert(a_tog && sel_tog && y_out);

        auto v10 = make_int_val(10);
        auto v_lo = make_bool_val(false);
        auto v_hi = make_bool_val(true);

        a_tog->update_value(&v10, DEFAULT_TTL);
        sel_tog->update_value(&v_lo, DEFAULT_TTL);
        run_queued_calculations();
        assert(y_out->get_value().intval == 15); // 10 + 5

        sel_tog->update_value(&v_hi, DEFAULT_TTL);
        run_queued_calculations();
        assert(y_out->get_value().intval == 5); // 10 - 5
    }

    // 2. Real Turbostar export simulation test (tests/data/register16.json)
    const std::string reg16_path = "tests/data/register16.json";
    assert(std::filesystem::exists(reg16_path));
    {
        verilog_importer::options opts;
        opts.use_clock_element = false;
        auto res = verilog_importer::import_file(reg16_path, opts);
        assert(res.ok);
        assert(res.imported_scene != nullptr);
        assert(res.top_module == "register16");
        assert(res.stats.ports == 7);
        assert(res.stats.constants == 4);
        assert(res.stats.components == 8);
        assert(res.stats.missing_endpoints == 0);
        assert(res.stats.warnings.empty());

        auto find_toggle = [&](std::string_view nm) -> model_toggle * {
            for (auto &e : res.imported_scene->elements) {
                if (e->class_id() == "model_toggle:" && e->get_name() == nm)
                    return static_cast<model_toggle *>(e.get());
            }
            return nullptr;
        };
        model_output *val_out = nullptr;
        for (auto &e : res.imported_scene->elements) {
            if (e->class_id() == "model_output:" && e->get_name() == "value")
                val_out = static_cast<model_output *>(e.get());
        }

        auto *clk  = find_toggle("clk");
        auto *rst  = find_toggle("rst");
        auto *load = find_toggle("load");
        auto *inc2 = find_toggle("inc2");
        auto *dec2 = find_toggle("dec2");
        auto *lval = find_toggle("load_value");
        assert(clk && rst && load && inc2 && dec2 && lval && val_out);

        auto v_lo = make_bool_val(false);
        auto v_hi = make_bool_val(true);
        auto tick = [&]() {
            clk->update_value(&v_lo, DEFAULT_TTL);
            run_queued_calculations();
            clk->update_value(&v_hi, DEFAULT_TTL);
            run_queued_calculations();
        };

        // Reset -> value = 0x0200, sp_plus_2 = 0x0202, sp_minus_2 = 0x01FE
        rst->update_value(&v_hi, DEFAULT_TTL);
        load->update_value(&v_lo, DEFAULT_TTL);
        inc2->update_value(&v_lo, DEFAULT_TTL);
        dec2->update_value(&v_lo, DEFAULT_TTL);
        run_queued_calculations();
        assert(val_out->get_value().intval == 0x0200);

        // Release reset, assert inc2 -> next clock loads sp_plus_2 (0x0202)
        rst->update_value(&v_lo, DEFAULT_TTL);
        inc2->update_value(&v_hi, DEFAULT_TTL);
        tick();
        assert(val_out->get_value().intval == 0x0202);

        // Assert load with 0x1234 -> next clock loads 0x1234
        auto v_1234 = make_int_val(0x1234);
        inc2->update_value(&v_lo, DEFAULT_TTL);
        load->update_value(&v_hi, DEFAULT_TTL);
        lval->update_value(&v_1234, DEFAULT_TTL);
        tick();
        assert(val_out->get_value().intval == 0x1234);
    }

    // 3. Self-contained full SoC import check (tests/data/top.json)
    const std::string top_path = "tests/data/top.json";
    assert(std::filesystem::exists(top_path));
    {
        verilog_importer::options opts;
        opts.load_sibling_files = false; // Must succeed purely self-contained!
        auto res = verilog_importer::import_file(top_path, opts);
        assert(res.ok);
        assert(res.imported_scene != nullptr);
        assert(res.stats.unsupported_components == 0);
        assert(res.stats.missing_endpoints == 0);
        assert(res.stats.warnings.empty());
    }

    // 4. model_mem ROM hex INIT + RAM write/read check
    {
        model_mem rom(0, 0, 4, 8, 16, false, false, false);
        rom.load_init_string("0x71795e397c776f7f077d6d664f5b063f");
        assert(rom.read_word(0x0) == 0x3f);
        assert(rom.read_word(0x1) == 0x06);
        assert(rom.read_word(0xF) == 0x71);
    }

    // 5. Multi-output gridcad_lut (model_truth with 2 inputs, 2 outputs)
    {
        const json multi_lut = {
            {"top_module", "mlut"},
            {"modules", {
                {"mlut", {
                    {"ports", {
                        {{"name", "a0"}, {"direction", "input"}, {"width", 1}, {"order", 0}},
                        {{"name", "a1"}, {"direction", "input"}, {"width", 1}, {"order", 1}},
                        {{"name", "y0"}, {"direction", "output"}, {"width", 1}, {"order", 2}},
                        {{"name", "y1"}, {"direction", "output"}, {"width", 1}, {"order", 3}}
                    }},
                    {"constants", json::array()},
                    {"components", {
                        {{"id", "lut0"}, {"type", "gridcad_lut"}, {"is_submodule", false},
                         {"parameters", {{"WIDTH", 2}, {"OUTPUTS", 2}, {"LUT", {"1001", "0100"}}}}}
                    }},
                    {"wires", {
                        {{"name", "a0"}, {"width", 1},
                         {"driver", {{"kind", "port"}, {"id", "a0"}, {"pin", "a0"}}},
                         {"sinks", {{{"kind", "component"}, {"id", "lut0"}, {"pin", "A0"}}}}},
                        {{"name", "a1"}, {"width", 1},
                         {"driver", {{"kind", "port"}, {"id", "a1"}, {"pin", "a1"}}},
                         {"sinks", {{{"kind", "component"}, {"id", "lut0"}, {"pin", "A1"}}}}},
                        {{"name", "y0"}, {"width", 1},
                         {"driver", {{"kind", "component"}, {"id", "lut0"}, {"pin", "Y0"}}},
                         {"sinks", {{{"kind", "port"}, {"id", "y0"}, {"pin", "y0"}}}}},
                        {{"name", "y1"}, {"width", 1},
                         {"driver", {{"kind", "component"}, {"id", "lut0"}, {"pin", "Y1"}}},
                         {"sinks", {{{"kind", "port"}, {"id", "y1"}, {"pin", "y1"}}}}}
                    }}
                }}
            }}
        };
        auto res = verilog_importer::import_json(multi_lut);
        assert(res.ok && res.stats.missing_endpoints == 0 && res.stats.warnings.empty());

        model_toggle *a0 = nullptr, *a1 = nullptr;
        model_output *y0 = nullptr, *y1 = nullptr;
        for (auto &e : res.imported_scene->elements) {
            if (e->class_id() == "model_toggle:" && e->get_name() == "a0") a0 = static_cast<model_toggle *>(e.get());
            if (e->class_id() == "model_toggle:" && e->get_name() == "a1") a1 = static_cast<model_toggle *>(e.get());
            if (e->class_id() == "model_output:" && e->get_name() == "y0") y0 = static_cast<model_output *>(e.get());
            if (e->class_id() == "model_output:" && e->get_name() == "y1") y1 = static_cast<model_output *>(e.get());
        }
        assert(a0 && a1 && y0 && y1);

        auto v0 = make_bool_val(false), v1 = make_bool_val(true);
        // a1=1, a0=0 -> idx=2 -> y0=("1001")[1]=0, y1=("0100")[1]=1
        a0->update_value(&v0, DEFAULT_TTL);
        a1->update_value(&v1, DEFAULT_TTL);
        run_queued_calculations();
        assert(y0->get_value().boolval == false);
        assert(y1->get_value().boolval == true);

        // a1=1, a0=1 -> idx=3 -> y0=("1001")[0]=1, y1=("0100")[0]=0
        a0->update_value(&v1, DEFAULT_TTL);
        run_queued_calculations();
        assert(y0->get_value().boolval == true);
        assert(y1->get_value().boolval == false);
    }

    // 6. model_slice + model_concat (16-bit byte swap: {in[7:0], in[15:8]})
    {
        scene s("slice_concat_test");
        auto in_bus = std::make_unique<model_toggle>(1, 2);
        auto lo_slc = std::make_unique<model_slice>(6, 1, 16, 8, 0);
        auto hi_slc = std::make_unique<model_slice>(6, 4, 16, 8, 8);
        auto cat    = std::make_unique<model_concat>(12, 2, std::vector<int>{8, 8}, 16);
        auto out_y  = std::make_unique<model_output>(18, 2);

        in_bus->set_width(16);
        out_y->set_width(16);

        auto *in_p = in_bus.get();
        auto *lo_p = lo_slc.get();
        auto *hi_p = hi_slc.get();
        auto *cat_p = cat.get();
        auto *out_p = out_y.get();

        s.add_element(std::move(in_bus));
        s.add_element(std::move(lo_slc));
        s.add_element(std::move(hi_slc));
        s.add_element(std::move(cat));
        s.add_element(std::move(out_y));

        connect_ports(in_p->port_at(0), lo_p->port_at(0));
        connect_ports(in_p->port_at(0), hi_p->port_at(0));
        // in0 (LSB) = hi byte, in1 (MSB) = lo byte -> swaps bytes
        connect_ports(hi_p->port_at(1), cat_p->port_at(0));
        connect_ports(lo_p->port_at(1), cat_p->port_at(1));
        connect_ports(cat_p->port_at(2), out_p->port_at(0));
        s.redo_nets();

        auto v_in = make_int_val(0x1234);
        in_p->update_value(&v_in, DEFAULT_TTL);
        run_queued_calculations();
        assert(out_p->get_value().intval == 0x3412);
    }

    // 7. model_split (4-bit bus -> 4 individual 1-bit outputs)
    {
        scene s("split_test");
        auto in_bus = std::make_unique<model_toggle>(1, 2);
        auto spl    = std::make_unique<model_split>(6, 2, 4, 4);
        auto o0     = std::make_unique<model_output>(12, 1);
        auto o2     = std::make_unique<model_output>(12, 5);
        auto o3     = std::make_unique<model_output>(12, 9);
        in_bus->set_width(4);

        auto *in_p = in_bus.get();
        auto *spl_p = spl.get();
        auto *o0_p = o0.get();
        auto *o2_p = o2.get();
        auto *o3_p = o3.get();

        s.add_element(std::move(in_bus));
        s.add_element(std::move(spl));
        s.add_element(std::move(o0));
        s.add_element(std::move(o2));
        s.add_element(std::move(o3));

        connect_ports(in_p->port_at(0), spl_p->port_at(0));
        connect_ports(spl_p->port_at(1), o0_p->port_at(0)); // bit 0
        connect_ports(spl_p->port_at(3), o2_p->port_at(0)); // bit 2
        connect_ports(spl_p->port_at(4), o3_p->port_at(0)); // bit 3
        s.redo_nets();

        auto v_a = make_int_val(0b1010);
        in_p->update_value(&v_a, DEFAULT_TTL);
        run_queued_calculations();
        assert(o0_p->get_value().boolval == false);
        assert(o2_p->get_value().boolval == false);
        assert(o3_p->get_value().boolval == true);
    }

    // 8. Wide bitwise gates (8-bit AND, OR, XOR, NOT)
    {
        scene s("wide_bitwise_test");
        auto in_a  = std::make_unique<model_toggle>(1, 1);
        auto in_b  = std::make_unique<model_toggle>(1, 6);
        auto g_and = std::make_unique<model_and>(7, 1);
        auto g_or  = std::make_unique<model_or>(7, 5);
        auto g_xor = std::make_unique<model_xor>(7, 9);
        auto g_not = std::make_unique<model_not>(7, 13);
        auto o_and = std::make_unique<model_output>(14, 1);
        auto o_or  = std::make_unique<model_output>(14, 5);
        auto o_xor = std::make_unique<model_output>(14, 9);
        auto o_not = std::make_unique<model_output>(14, 13);

        in_a->set_width(8);
        in_b->set_width(8);
        g_and->set_bus_width(8);
        g_or->set_bus_width(8);
        g_xor->set_bus_width(8);
        g_not->set_bus_width(8);
        o_and->set_width(8);
        o_or->set_width(8);
        o_xor->set_width(8);
        o_not->set_width(8);

        auto *a_p = in_a.get();
        auto *b_p = in_b.get();
        auto *and_p = g_and.get();
        auto *or_p = g_or.get();
        auto *xor_p = g_xor.get();
        auto *not_p = g_not.get();
        auto *oa_p = o_and.get();
        auto *oo_p = o_or.get();
        auto *ox_p = o_xor.get();
        auto *on_p = o_not.get();

        s.add_element(std::move(in_a));
        s.add_element(std::move(in_b));
        s.add_element(std::move(g_and));
        s.add_element(std::move(g_or));
        s.add_element(std::move(g_xor));
        s.add_element(std::move(g_not));
        s.add_element(std::move(o_and));
        s.add_element(std::move(o_or));
        s.add_element(std::move(o_xor));
        s.add_element(std::move(o_not));

        connect_ports(a_p->port_at(0), and_p->port_at(0));
        connect_ports(b_p->port_at(0), and_p->port_at(1));
        connect_ports(and_p->port_at(2), oa_p->port_at(0));

        connect_ports(a_p->port_at(0), or_p->port_at(0));
        connect_ports(b_p->port_at(0), or_p->port_at(1));
        connect_ports(or_p->port_at(2), oo_p->port_at(0));

        connect_ports(a_p->port_at(0), xor_p->port_at(0));
        connect_ports(b_p->port_at(0), xor_p->port_at(1));
        connect_ports(xor_p->port_at(2), ox_p->port_at(0));

        connect_ports(a_p->port_at(0), not_p->port_at(0));
        connect_ports(not_p->port_at(1), on_p->port_at(0));
        s.redo_nets();

        auto va = make_int_val(0xA5);
        auto vb = make_int_val(0x3C);
        a_p->update_value(&va, DEFAULT_TTL);
        b_p->update_value(&vb, DEFAULT_TTL);
        run_queued_calculations();

        assert(oa_p->get_value().intval == (0xA5 & 0x3C));
        assert(oo_p->get_value().intval == (0xA5 | 0x3C));
        assert(ox_p->get_value().intval == (0xA5 ^ 0x3C));
        assert(on_p->get_value().intval == ((~0xA5u) & 0xFFu));
    }

    // 9. model_gt and model_ge (4-bit unsigned comparators)
    {
        scene s("cmp_gt_ge_test");
        auto in_a = std::make_unique<model_toggle>(1, 1);
        auto in_b = std::make_unique<model_toggle>(1, 5);
        auto gt   = std::make_unique<model_gt>(7, 1, 4);
        auto ge   = std::make_unique<model_ge>(7, 5, 4);
        auto o_gt = std::make_unique<model_output>(13, 1);
        auto o_ge = std::make_unique<model_output>(13, 5);

        in_a->set_width(4);
        in_b->set_width(4);

        auto *a_p = in_a.get();
        auto *b_p = in_b.get();
        auto *gt_p = gt.get();
        auto *ge_p = ge.get();
        auto *ogt_p = o_gt.get();
        auto *oge_p = o_ge.get();

        s.add_element(std::move(in_a));
        s.add_element(std::move(in_b));
        s.add_element(std::move(gt));
        s.add_element(std::move(ge));
        s.add_element(std::move(o_gt));
        s.add_element(std::move(o_ge));

        connect_ports(a_p->port_at(0), gt_p->port_at(0));
        connect_ports(b_p->port_at(0), gt_p->port_at(1));
        connect_ports(gt_p->port_at(2), ogt_p->port_at(0));

        connect_ports(a_p->port_at(0), ge_p->port_at(0));
        connect_ports(b_p->port_at(0), ge_p->port_at(1));
        connect_ports(ge_p->port_at(2), oge_p->port_at(0));
        s.redo_nets();

        auto v9 = make_int_val(9);
        auto v9b = make_int_val(9);
        a_p->update_value(&v9, DEFAULT_TTL);
        b_p->update_value(&v9b, DEFAULT_TTL);
        run_queued_calculations();
        assert(ogt_p->get_value().boolval == false);
        assert(oge_p->get_value().boolval == true);

        auto v10 = make_int_val(10);
        a_p->update_value(&v10, DEFAULT_TTL);
        run_queued_calculations();
        assert(ogt_p->get_value().boolval == true);
        assert(oge_p->get_value().boolval == true);

        auto v5 = make_int_val(5);
        a_p->update_value(&v5, DEFAULT_TTL);
        run_queued_calculations();
        assert(ogt_p->get_value().boolval == false);
        assert(oge_p->get_value().boolval == false);
    }

    // 10. Packed vector LUT (mixed-width inputs: A0=2-bit, A1=1-bit -> 3-bit index, 2 outputs)
    {
        scene s("packed_lut_test");
        auto in_a0 = std::make_unique<model_toggle>(1, 1);
        auto in_a1 = std::make_unique<model_toggle>(1, 5);
        auto lut   = std::make_unique<model_truth>(7, 2);
        auto o0    = std::make_unique<model_output>(14, 1);
        auto o1    = std::make_unique<model_output>(14, 5);

        in_a0->set_width(2);
        // idx = (A1 << 2) | A0 in [0..7]; MSB-first bitstring has idx=5 at pos (7-5)=2
        // Y0 = 1 only for idx == 5 (A1=1, A0=1); Y1 = 1 only for idx == 2 (A1=0, A0=2)
        lut->configure_lut_packed({2, 1}, {"A0", "A1"}, {"00100000", "00000100"});

        auto *a0_p = in_a0.get();
        auto *a1_p = in_a1.get();
        auto *lut_p = lut.get();
        auto *o0_p = o0.get();
        auto *o1_p = o1.get();

        s.add_element(std::move(in_a0));
        s.add_element(std::move(in_a1));
        s.add_element(std::move(lut));
        s.add_element(std::move(o0));
        s.add_element(std::move(o1));

        connect_ports(a0_p->port_at(0), lut_p->port_at(0));
        connect_ports(a1_p->port_at(0), lut_p->port_at(1));
        connect_ports(lut_p->port_at(2), o0_p->port_at(0));
        connect_ports(lut_p->port_at(3), o1_p->port_at(0));
        s.redo_nets();

        auto v1 = make_int_val(1);
        auto b1 = make_bool_val(true);
        a0_p->update_value(&v1, DEFAULT_TTL);
        a1_p->update_value(&b1, DEFAULT_TTL);
        run_queued_calculations();
        assert(o0_p->get_value().boolval == true);
        assert(o1_p->get_value().boolval == false);

        auto v2 = make_int_val(2);
        auto b0 = make_bool_val(false);
        a0_p->update_value(&v2, DEFAULT_TTL);
        a1_p->update_value(&b0, DEFAULT_TTL);
        run_queued_calculations();
        assert(o0_p->get_value().boolval == false);
        assert(o1_p->get_value().boolval == true);
    }

    // 11. model_pmux (16-bit 3-way priority/one-hot mux)
    {
        scene s("pmux_test");
        auto in_a  = std::make_unique<model_toggle>(1, 1);
        auto in_b0 = std::make_unique<model_toggle>(1, 4);
        auto in_b1 = std::make_unique<model_toggle>(1, 7);
        auto in_b2 = std::make_unique<model_toggle>(1, 10);
        auto s0    = std::make_unique<model_toggle>(1, 14);
        auto s1    = std::make_unique<model_toggle>(1, 17);
        auto s2    = std::make_unique<model_toggle>(1, 20);
        auto pmux  = std::make_unique<model_pmux>(10, 4, 16, 3);
        auto out_y = std::make_unique<model_output>(18, 6);

        in_a->set_width(16);
        in_b0->set_width(16);
        in_b1->set_width(16);
        in_b2->set_width(16);
        out_y->set_width(16);

        auto *a_p = in_a.get();
        auto *b0_p = in_b0.get();
        auto *b1_p = in_b1.get();
        auto *b2_p = in_b2.get();
        auto *s0_p = s0.get();
        auto *s1_p = s1.get();
        auto *s2_p = s2.get();
        auto *pm_p = pmux.get();
        auto *y_p = out_y.get();

        s.add_element(std::move(in_a));
        s.add_element(std::move(in_b0));
        s.add_element(std::move(in_b1));
        s.add_element(std::move(in_b2));
        s.add_element(std::move(s0));
        s.add_element(std::move(s1));
        s.add_element(std::move(s2));
        s.add_element(std::move(pmux));
        s.add_element(std::move(out_y));

        connect_ports(a_p->port_at(0),  pm_p->port_at(0));
        connect_ports(b0_p->port_at(0), pm_p->port_at(1));
        connect_ports(b1_p->port_at(0), pm_p->port_at(2));
        connect_ports(b2_p->port_at(0), pm_p->port_at(3));
        connect_ports(s0_p->port_at(0), pm_p->port_at(4));
        connect_ports(s1_p->port_at(0), pm_p->port_at(5));
        connect_ports(s2_p->port_at(0), pm_p->port_at(6));
        connect_ports(pm_p->port_at(7), y_p->port_at(0));
        s.redo_nets();

        auto va = make_int_val(0xAAAA), vb0 = make_int_val(0x1111),
             vb1 = make_int_val(0x2222), vb2 = make_int_val(0x3333);
        auto v0 = make_bool_val(false), v1 = make_bool_val(true);
        a_p->update_value(&va, DEFAULT_TTL);
        b0_p->update_value(&vb0, DEFAULT_TTL);
        b1_p->update_value(&vb1, DEFAULT_TTL);
        b2_p->update_value(&vb2, DEFAULT_TTL);
        s0_p->update_value(&v0, DEFAULT_TTL);
        s1_p->update_value(&v0, DEFAULT_TTL);
        s2_p->update_value(&v0, DEFAULT_TTL);
        run_queued_calculations();
        assert(y_p->get_value().intval == 0xAAAA);

        s1_p->update_value(&v1, DEFAULT_TTL);
        run_queued_calculations();
        assert(y_p->get_value().intval == 0x2222);

        s1_p->update_value(&v0, DEFAULT_TTL);
        s2_p->update_value(&v1, DEFAULT_TTL);
        run_queued_calculations();
        assert(y_p->get_value().intval == 0x3333);
    }

    // 12. model_reduce (4-bit OR, AND, XOR, XNOR reductions)
    {
        scene s("reduce_test");
        auto in_a  = std::make_unique<model_toggle>(1, 4);
        auto r_or  = std::make_unique<model_reduce>(7, 1, 4, reduce_op::OR);
        auto r_and = std::make_unique<model_reduce>(7, 5, 4, reduce_op::AND);
        auto r_xor = std::make_unique<model_reduce>(7, 9, 4, reduce_op::XOR);
        auto r_xnr = std::make_unique<model_reduce>(7, 13, 4, reduce_op::XNOR);
        auto o_or  = std::make_unique<model_output>(13, 1);
        auto o_and = std::make_unique<model_output>(13, 5);
        auto o_xor = std::make_unique<model_output>(13, 9);
        auto o_xnr = std::make_unique<model_output>(13, 13);

        in_a->set_width(4);

        auto *a_p = in_a.get();
        auto *ror_p = r_or.get(), *rand_p = r_and.get(), *rxor_p = r_xor.get(), *rxnr_p = r_xnr.get();
        auto *oor_p = o_or.get(), *oand_p = o_and.get(), *oxor_p = o_xor.get(), *oxnr_p = o_xnr.get();

        s.add_element(std::move(in_a));
        s.add_element(std::move(r_or));
        s.add_element(std::move(r_and));
        s.add_element(std::move(r_xor));
        s.add_element(std::move(r_xnr));
        s.add_element(std::move(o_or));
        s.add_element(std::move(o_and));
        s.add_element(std::move(o_xor));
        s.add_element(std::move(o_xnr));

        connect_ports(a_p->port_at(0), ror_p->port_at(0));
        connect_ports(a_p->port_at(0), rand_p->port_at(0));
        connect_ports(a_p->port_at(0), rxor_p->port_at(0));
        connect_ports(a_p->port_at(0), rxnr_p->port_at(0));
        connect_ports(ror_p->port_at(1),  oor_p->port_at(0));
        connect_ports(rand_p->port_at(1), oand_p->port_at(0));
        connect_ports(rxor_p->port_at(1), oxor_p->port_at(0));
        connect_ports(rxnr_p->port_at(1), oxnr_p->port_at(0));
        s.redo_nets();

        auto v0 = make_int_val(0b0000);
        a_p->update_value(&v0, DEFAULT_TTL);
        run_queued_calculations();
        assert(oor_p->get_value().boolval == false);
        assert(oand_p->get_value().boolval == false);
        assert(oxor_p->get_value().boolval == false);
        assert(oxnr_p->get_value().boolval == true);

        auto v7 = make_int_val(0b0111); // odd popcount (3)
        a_p->update_value(&v7, DEFAULT_TTL);
        run_queued_calculations();
        assert(oor_p->get_value().boolval == true);
        assert(oand_p->get_value().boolval == false);
        assert(oxor_p->get_value().boolval == true);
        assert(oxnr_p->get_value().boolval == false);

        auto vf = make_int_val(0b1111); // even popcount (4), all ones
        a_p->update_value(&vf, DEFAULT_TTL);
        run_queued_calculations();
        assert(oor_p->get_value().boolval == true);
        assert(oand_p->get_value().boolval == true);
        assert(oxor_p->get_value().boolval == false);
        assert(oxnr_p->get_value().boolval == true);
    }
}

static void test_vcd_playback(void)
{
    // 1. Wire VCD-lock prevents simulation overwrite while unlocked glue wires still simulate
    {
        scene s("vcd_lock_test");
        auto in_a  = std::make_unique<model_toggle>(1, 2);
        auto in_b  = std::make_unique<model_toggle>(1, 6);
        auto adder = std::make_unique<model_add>(8, 4, 8);
        auto out_y = std::make_unique<model_output>(16, 4);

        in_a->set_width(8);
        in_b->set_width(8);
        out_y->set_width(8);

        auto *a_p = in_a.get();
        auto *b_p = in_b.get();
        auto *add_p = adder.get();
        auto *y_p = out_y.get();

        s.add_element(std::move(in_a));
        s.add_element(std::move(in_b));
        s.add_element(std::move(adder));
        s.add_element(std::move(out_y));

        auto wa = std::make_shared<wire>(0, 0, 0, 0);
        wa->set_vcd_name("a");
        wa->set_width(8);
        a_p->port_at(0)->add_wire(wa);
        add_p->port_at(0)->add_wire(wa);

        auto wb = std::make_shared<wire>(0, 0, 0, 0);
        wb->set_vcd_name("b");
        wb->set_width(8);
        b_p->port_at(0)->add_wire(wb);
        add_p->port_at(1)->add_wire(wb);

        // Glue wire (no vcd_name) -> must remain unlocked and compute a + b
        auto wy = std::make_shared<wire>(0, 0, 0, 0);
        wy->set_width(8);
        add_p->port_at(2)->add_wire(wy);
        y_p->port_at(0)->add_wire(wy);

        s.redo_nets();

        // Broadcast VCD values a=0x10, b=0x25 -> locks a and b, glue wire y computes 0x35
        assert(s.broadcast_vcd_value("a", make_int_val(0x10)) == 1);
        assert(s.broadcast_vcd_value("b", make_int_val(0x25)) == 1);
        run_queued_calculations();
        assert(wa->is_vcd_locked());
        assert(wb->is_vcd_locked());
        assert(!wy->is_vcd_locked());
        assert(y_p->get_value().intval == 0x35);

        // Normal simulation write to locked wire 'a' must be ignored!
        auto v_override_attempt = make_int_val(0x99);
        a_p->update_value(&v_override_attempt, DEFAULT_TTL);
        run_queued_calculations();
        assert(wa->value.intval == 0x10);
        assert(y_p->get_value().intval == 0x35);

        // Updating VCD value on 'a' to 0x40 updates both 'a' and unlocked glue wire 'y' -> 0x65
        assert(s.broadcast_vcd_value("a", make_int_val(0x40)) == 1);
        run_queued_calculations();
        assert(y_p->get_value().intval == 0x65);

        // Unlocking restores normal simulation control
        s.unlock_all_vcd();
        assert(!wa->is_vcd_locked());
        a_p->update_value(&v_override_attempt, DEFAULT_TTL);
        run_queued_calculations();
        assert(y_p->get_value().intval == ((0x99 + 0x25) & 0xFF));
    }

    // 1b. Regression: unsplicing ("Join wire") a connector on a VCD net must not leave dangling pointers
    {
        scene s("unsplice_vcd_test");
        auto in_a  = std::make_unique<model_toggle>(1, 2);
        auto conn  = std::make_unique<connector>(6, 2);
        auto out_y = std::make_unique<model_output>(12, 2);
        in_a->set_width(8);
        out_y->set_width(8);

        auto *a_p = in_a.get();
        auto *c_p = conn.get();
        auto *y_p = out_y.get();
        s.add_element(std::move(in_a));
        s.add_element(std::move(conn));
        s.add_element(std::move(out_y));

        auto w1 = std::make_shared<wire>(0, 0, 0, 0);
        w1->name = "bus_sig";
        w1->set_vcd_name("bus_sig");
        w1->set_width(8);
        a_p->port_at(0)->add_wire(w1);
        c_p->port_at(0)->add_wire(w1);

        auto w2 = std::make_shared<wire>(0, 0, 0, 0);
        w2->name = "bus_sig";
        w2->set_vcd_name("bus_sig");
        w2->set_width(8);
        c_p->port_at(0)->add_wire(w2);
        y_p->port_at(0)->add_wire(w2);

        s.redo_nets();
        assert(s.broadcast_vcd_value("bus_sig", make_int_val(0x42)) == 1);
        assert(y_p->get_value().intval == 0x42);

        // Unsplice connector and delete it WITHOUT calling redo_nets()
        c_p->unsplice();
        s.process_delete_requests();

        // Subsequent VCD broadcast must cleanly update the joined wire with no UAF
        assert(s.broadcast_vcd_value("bus_sig", make_int_val(0x7E)) == 1);
        run_queued_calculations();
        assert(y_p->get_value().intval == 0x7E);
    }

    // 1c. scene::redo_net() multi-source Steiner net rebuild preserves metadata & connectivity
    {
        scene s("redo_net_test");
        auto in_a  = std::make_unique<model_toggle>(4, 20);
        auto bad_c = std::make_unique<connector>(2, 2); // deliberately bad corner
        auto o1    = std::make_unique<model_output>(40, 20); // pure horizontal far sink
        auto o2    = std::make_unique<model_output>(20, 10); // middle-top sink
        auto o3    = std::make_unique<model_output>(30, 30); // middle-bottom sink
        in_a->set_width(8);
        o1->set_width(8);
        o2->set_width(8);
        o3->set_width(8);

        auto *a_p = in_a.get();
        auto *c_p = bad_c.get();
        auto *o1_p = o1.get(), *o2_p = o2.get(), *o3_p = o3.get();
        s.add_element(std::move(in_a));
        s.add_element(std::move(bad_c));
        s.add_element(std::move(o1));
        s.add_element(std::move(o2));
        s.add_element(std::move(o3));

        auto add_sig_wire = [&](port *p1, port *p2) {
            auto w = std::make_shared<wire>(0, 0, 0, 0, 6);
            w->name = "data_bus";
            w->set_vcd_name("dut.data_bus");
            w->set_src_info("src/foo.sv:42", "logic [7:0] data_bus;");
            w->set_width(8);
            p1->add_wire(w);
            p2->add_wire(w);
            w->push_wire_color(6);
        };
        add_sig_wire(a_p->port_at(0), c_p->port_at(0));
        add_sig_wire(c_p->port_at(0), o1_p->port_at(0));
        add_sig_wire(c_p->port_at(0), o2_p->port_at(0));
        add_sig_wire(c_p->port_at(0), o3_p->port_at(0));
        s.redo_nets();

        s.redo_net(c_p);

        // Verify all 3 outputs receive VCD broadcast and retain wire metadata
        assert(s.broadcast_vcd_value("dut.data_bus", make_int_val(0xB5)) == 1);
        run_queued_calculations();
        assert(o1_p->get_value().intval == 0xB5);
        assert(o2_p->get_value().intval == 0xB5);
        assert(o3_p->get_value().intval == 0xB5);

        for (port *op : {o1_p->port_at(0), o2_p->port_at(0), o3_p->port_at(0)}) {
            assert(!op->get_wires().empty());
            auto w = op->get_wires()[0];
            assert(w->name == "data_bus");
            assert(w->get_color() == 6);
            assert(w->get_vcd_name() == "dut.data_bus");
            assert(w->get_src_location() == "src/foo.sv:42");
            assert(w->get_src_verilog() == "logic [7:0] data_bus;");
        }
    }

    // 1d. layout_overlay save/apply: element positions, wire colors, area labels, and datascopes
    {
        const std::filesystem::path tmp_layout =
            std::filesystem::temp_directory_path() / "gridcad_test_layout_overlay.json";
        std::error_code ec;
        std::filesystem::remove(tmp_layout, ec);

        // Import register16, edit layout (move an element, recolor a wire, add label + hex datascope)
        auto res1 = verilog_importer::import_file("tests/data/register16.json");
        assert(res1.ok && res1.imported_scene != nullptr);
        scene *s1 = res1.imported_scene.get();

        element *val_elem = nullptr;
        port *val_port = nullptr;
        for (auto &e : s1->elements) {
            if (e->class_id() == "model_output:" && e->get_name() == "value") {
                val_elem = e.get();
                val_port = e->port_at(0);
                break;
            }
        }
        assert(val_elem && val_port && !val_port->get_wires().empty());
        val_elem->place(77, 19);
        val_elem->reseat();
        val_port->push_net_color(4);

        auto lbl = std::make_unique<model_label>(12, 8);
        lbl->update_name("sp_plus_2");
        lbl->set_size(14, 7);
        lbl->set_color(3);
        lbl->set_bottomlabel(false);
        s1->add_element(std::move(lbl));

        auto ds = std::make_unique<model_datascope>(60, 25);
        ds->update_name("val_scope");
        ds->set_format(1); // hex mode
        model_datascope *ds_p = ds.get();
        s1->add_element(std::move(ds));

        // Tap datascope onto 'value' net using a manually-drawn default-named ("Wire-...") wire
        auto tap_w = std::make_shared<wire>(0, 0, 0, 0, 4);
        tap_w->set_width(16);
        val_port->get_wires()[0]->get_other_port(val_port)->add_wire(tap_w);
        ds_p->port_at(0)->add_wire(tap_w);
        // Tap a second datascope onto an unnamed 1-to-1 internal wire (gridcad_mux_2:y -> gridcad_mux_3:a)
        element *mux2 = nullptr;
        for (auto &e : s1->elements) {
            if (e->class_id() == "model_mux:" && e->get_name() == "gridcad_mux_2") {
                mux2 = e.get();
                break;
            }
        }
        assert(mux2 && mux2->port_at(3));
        auto ds2 = std::make_unique<model_datascope>(79, 11);
        ds2->update_name("mux_scope");
        ds2->set_format(1);
        model_datascope *ds2_p = ds2.get();
        s1->add_element(std::move(ds2));
        auto tap_w2 = std::make_shared<wire>(0, 0, 0, 0, 0);
        tap_w2->set_width(16);
        mux2->port_at(3)->add_wire(tap_w2);
        ds2_p->port_at(0)->add_wire(tap_w2);

        s1->redo_nets();
        s1->mark_layout_dirty();

        assert(layout_overlay::save_scene_to_file(s1, tmp_layout));
        assert(!s1->is_layout_dirty());

        // Fresh import of register16 + apply saved layout overlay
        auto res2 = verilog_importer::import_file("tests/data/register16.json");
        assert(res2.ok && res2.imported_scene != nullptr);
        scene *s2 = res2.imported_scene.get();

        assert(layout_overlay::apply_file_to_scene(s2, tmp_layout));
        s2->redo_all_multi_sink_nets();
        s2->redo_nets();
        assert(!s2->is_layout_dirty());

        bool found_moved_val = false, found_lbl = false;
        int found_connected_ds = 0;
        for (auto &e : s2->elements) {
            if (e->class_id() == "model_output:" && e->get_name() == "value") {
                assert(static_cast<int>(std::round(e->get_X())) == 77);
                assert(static_cast<int>(std::round(e->get_Y())) == 19);
                assert(!e->port_at(0)->get_wires().empty());
                assert(e->port_at(0)->get_wires()[0]->get_color() == 4);
                found_moved_val = true;
            } else if (e->class_id() == "model_label:" && e->get_name() == "sp_plus_2") {
                auto *l = static_cast<model_label *>(e.get());
                assert(static_cast<int>(std::round(l->get_X())) == 12);
                assert(static_cast<int>(std::round(l->get_Y())) == 8);
                assert(l->get_color() == 3);
                assert(l->get_bottomlabel() == false);
                found_lbl = true;
            } else if (e->class_id() == "model_datascope:") {
                auto *d = static_cast<model_datascope *>(e.get());
                assert(d->get_format() == 1);
                assert(d->port_at(0) && !d->port_at(0)->get_wires().empty());
                found_connected_ds++;
            }
        }
        assert(found_moved_val && found_lbl && found_connected_ds == 2);

        std::filesystem::remove(tmp_layout, ec);
    }

    // 1e. Undo JSON round-trip on hierarchical Verilog import (top.json):
    // verifies wire_id keying, scoped model_nest deserialization, VCD/src/layout metadata, and colors.
    {
        auto res = verilog_importer::import_file("tests/data/top.json");
        assert(res.ok && res.imported_scene != nullptr);
        scene *orig = res.imported_scene.get();
        orig->set_layout_project_key("proj_undo_test");
        orig->mark_layout_dirty();

        auto collect_stats = [](scene *sc, size_t &elem_cnt, size_t &wire_cnt,
                                size_t &vcd_wire_cnt, size_t &clk_color_cnt,
                                size_t &src_elem_cnt) {
            elem_cnt = sc->elements.size();
            std::set<const wire *> seen;
            wire_cnt = 0;
            vcd_wire_cnt = 0;
            clk_color_cnt = 0;
            src_elem_cnt = 0;
            for (const auto &e : sc->elements) {
                if (!e->get_src_location().empty())
                    src_elem_cnt++;
                for (unsigned int pi = 0; ; ++pi) {
                    port *p = e->port_at(pi);
                    if (!p)
                        break;
                    for (const auto &w : p->get_wires()) {
                        if (!w)
                            continue;
                        if (seen.insert(w.get()).second) {
                            assert(w->get_ports().size() == 2);
                            wire_cnt++;
                            if (!w->get_vcd_name().empty())
                                vcd_wire_cnt++;
                            if (w->get_color() == 1)
                                clk_color_cnt++;
                        }
                    }
                }
            }
        };

        size_t e1, w1, v1, c1, s1;
        collect_stats(orig, e1, w1, v1, c1, s1);
        assert(w1 > 10 && v1 > 0 && c1 > 0);

        json snap;
        orig->to_json(snap);
        std::string dumped = snap.dump();

        auto restored = std::make_unique<scene>(orig->get_name());
        json parsed = json::parse(dumped);
        restored->from_json(parsed);

        size_t e2, w2, v2, c2, s2;
        collect_stats(restored.get(), e2, w2, v2, c2, s2);
        assert(e2 == e1);
        assert(w2 == w1);
        assert(v2 == v1);
        assert(c2 == c1);
        assert(s2 == s1);
        auto count_ports = [](element *e) {
            unsigned int n = 0;
            while (e->port_at(n) != nullptr)
                ++n;
            return n;
        };
        for (size_t i = 0; i < orig->elements.size(); ++i)
            assert(count_ports(orig->elements[i].get()) == count_ports(restored->elements[i].get()));
        assert(restored->get_layout_project_key() == "proj_undo_test");
        assert(restored->get_layout_module_name() == orig->get_layout_module_name());
        assert(restored->is_layout_dirty());

        // Also verify nested submodules inside top.json kept every wire intact
        int nest_checked = 0;
        for (size_t i = 0; i < orig->elements.size(); ++i) {
            if (orig->elements[i]->class_id() != "model_nest:")
                continue;
            auto *n1 = static_cast<model_nest *>(orig->elements[i].get());
            auto *n2 = static_cast<model_nest *>(restored->elements[i].get());
            size_t ne1, nw1, nv1, nc1, ns1;
            size_t ne2, nw2, nv2, nc2, ns2;
            collect_stats(n1->get_scene(), ne1, nw1, nv1, nc1, ns1);
            collect_stats(n2->get_scene(), ne2, nw2, nv2, nc2, ns2);
            assert(ne2 == ne1);
            assert(nw2 == nw1);
            assert(nv2 == nv1);
            assert(nc2 == nc1);
            nest_checked++;
        }
        assert(nest_checked > 0);
    }

    // 2. Real test8080 top.json + dump_top_tb.vcd end-to-end binding & tick stepping
    const std::string top_path = "tests/data/top.json";
    const std::string vcd_path = "tests/data/dump_top_tb.vcd";
    assert(std::filesystem::exists(top_path));
    assert(std::filesystem::exists(vcd_path));
    {
        auto res = verilog_importer::import_file(top_path);
        assert(res.ok && res.imported_scene != nullptr);

        // Verify connector placement invariants, trunk splitting, and wire colors on top.json
        auto check_connectors_clear = [](scene *sc) {
            for (const auto &conn : sc->elements) {
                if (conn->class_id() != "connector:")
                    continue;
                // High-fanout connectors are split into trunks of degree <= 4
                assert(conn->port_at(0)->get_wires().size() <= 4);
                const float cx = conn->get_X();
                const float cy = conn->get_Y();
                for (const auto &other : sc->elements) {
                    if (other.get() == conn.get() || other->is_background() ||
                        other->class_id() == "connector:")
                        continue;
                    const float ox = other->get_X();
                    const float oy = other->get_Y();
                    const float ow = other->get_width();
                    const float oh = other->get_height();
                    // Connector must never overlap a component body or its left/right port columns
                    const bool overlaps = (cx >= ox - 1.0f && cx <= ox + ow &&
                                           cy >= oy && cy < oy + oh);
                    assert(!overlaps);
                }
            }
        };
        check_connectors_clear(res.imported_scene.get());
        topological_placer::place_connectors(res.imported_scene.get());
        check_connectors_clear(res.imported_scene.get());

        // Verify automatic wire coloring: clk=1, named signals=6, internal wire_*=0
        bool saw_clk = false, saw_named = false, saw_internal = false;
        for (const auto &elem : res.imported_scene->elements) {
            for (unsigned int pi = 0;; ++pi) {
                port *p = elem->port_at(pi);
                if (!p)
                    break;
                for (const auto &w : p->get_wires()) {
                    if (!w)
                        continue;
                    if (w->name == "clk") {
                        assert(w->get_color() == 1);
                        assert(!w->get_src_location().empty());
                        assert(!w->get_src_verilog().empty());
                        saw_clk = true;
                    } else if (w->name == "rst_n") {
                        assert(w->get_color() == 2);
                    } else if (w->name == "mem_read_data") {
                        assert(w->get_color() == 6);
                        saw_named = true;
                    } else if (w->name.starts_with("wire_")) {
                        assert(w->get_color() == 0);
                        saw_internal = true;
                    }
                }
            }
        }
        assert(saw_clk && saw_named && saw_internal);

        // Verify imported elements carry src_location & src_verilog metadata
        bool saw_elem_src = false;
        for (const auto &elem : res.imported_scene->elements) {
            if (!elem->get_src_location().empty() && !elem->get_src_verilog().empty()) {
                saw_elem_src = true;
                break;
            }
        }
        assert(saw_elem_src);

        vcd_session sess;
        std::string err;
        assert(sess.load(vcd_path, err));
        assert(sess.get_total_ticks() == 24);
        assert(sess.get_clock_signal() == "top_tb.clk");

        size_t bound = sess.bind_scene(res.imported_scene.get());
        assert(sess.get_scope_prefix() == "top_tb.dut");
        assert(bound >= 250);

        size_t applied0 = sess.apply_tick(0, res.imported_scene.get());
        assert(applied0 >= 250);

        size_t applied10 = sess.apply_tick(10, res.imported_scene.get());
        assert(applied10 >= 250);
        assert(sess.get_current_tick() == 10);

        // Verify timelinebar scrubber coordinate -> tick mapping
        timelinebar tb;
        tb.set_session(&sess);
        assert(tb.is_active());
        SDL_Rect bar = {50, 716, 900, timelinebar::BAR_HEIGHT};
        assert(tb.tick_from_x(0, bar) == 0);
        assert(tb.tick_from_x(1000, bar) == 23);
        size_t mid = tb.tick_from_x(bar.x + bar.w / 2, bar);
        assert(mid > 5 && mid < 18);
    }
}

int main(void)
{
    start_time = std::chrono::steady_clock::now();
    logger::get().set_level(log_level::warning);

    test_factory();
    test_logic_gates();
    test_parameterized_elements();
    test_verilog_import();
    test_vcd_playback();
    test_wirepath_routing();
    test_scene_json_and_verilog();

    std::cout << "All logic tests passed." << std::endl;
    return 0;
}
