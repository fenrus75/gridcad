/*
 * gridcad -- a tool to do digital circuit design with a GUI
 *
 * (C) Copyright 2025  -- Arjan van de Ven
 *
 * Licensed under the terms of the
 * GNU General Public LIcense version 3.0
 */

#pragma once

#include "gridcad.h"
#include "element.h"
#include "value.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

/* Parameterized ROM/RAM memory element matching Yosys/Turbostar gridcad_mem. */
class model_mem : public element
{
public:
    model_mem(float _X, float _Y, int abits = 4, int data_width = 8, int size = 16,
              bool has_write = false, bool rd_sync = false, bool wr_sync = true,
              int wr_en_width = 1);

    virtual std::string class_id(void) override { return "model_mem:"; }

    void drawAt(class basecanvas *canvas, float X, float Y, int type) override;
    void calculate(int ttl) override;
    void queued_calculate(int ttl) override;
    void to_json(json &j) override;
    void from_json(json &j) override;

    /* Load untrusted INIT string (hex "0x..." or bitstring "01x...", LSB = word 0 bit 0). */
    void load_init_string(std::string_view init_str);

    uint64_t read_word(size_t addr) const;
    size_t get_size(void) const { return mem.size(); }
    int get_data_width(void) const { return data_width; }
    bool has_write_port(void) const { return has_write; }

private:
    int abits = 4;
    int data_width = 8;
    int mem_size = 16;
    bool has_write = false;
    bool rd_sync = false;
    bool wr_sync = true;
    int wr_en_width = 1;

    bool prev_rd_clk = false;
    bool prev_wr_clk = false;
    uint64_t pending_write_addr = 0;
    uint64_t pending_write_data = 0;
    uint64_t pending_write_mask = 0;
    bool has_pending_write = false;
    struct value latched_rd_data = {};

    std::vector<uint64_t> mem;
};
