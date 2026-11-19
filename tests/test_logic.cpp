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

#include <cassert>
#include <iostream>
#include <memory>
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
        "model_nest:", "sequencer:", "connector:"
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

int main(void)
{
    start_time = std::chrono::steady_clock::now();
    logger::get().set_level(log_level::warning);

    test_factory();
    test_logic_gates();
    test_wirepath_routing();
    test_scene_json_and_verilog();

    std::cout << "All logic tests passed." << std::endl;
    return 0;
}
