/*
 * gridcad -- a tool to do digital circuit design with a GUI
 *
 * (C) Copyright 2025  -- Arjan van de Ven
 *
 * Licensed under the terms of the
 * GNU General Public LIcense version 3.0
 */

#include "gridcad.h"
#include "model_mem.h"
#include "port.h"

#include <algorithm>
#include <format>

namespace {

constexpr size_t MAX_MEM_WORDS = 65536;

uint64_t width_mask(int width)
{
    if (width <= 0)
        return 0;
    if (width >= 64)
        return ~uint64_t{0};
    return (uint64_t{1} << width) - 1;
}

uint64_t read_u64(const struct value &v, int width)
{
    uint64_t raw = (v.type == VALUE_TYPE_BOOL) ? (v.boolval ? 1ULL : 0ULL) : v.intval;
    return raw & width_mask(width);
}

int hex_nibble(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F')
        return 10 + (c - 'A');
    return 0;
}

} // namespace

model_mem::model_mem(float _X, float _Y, int _abits, int _data_width, int _size,
                     bool _has_write, bool _rd_sync, bool _wr_sync, int _wr_en_width)
    : element(4, 6, _has_write ? "RAM" : "ROM")
{
    sizeX = 4;
    sizeY = _has_write ? 8 : 6;
    X = floorf(_X);
    Y = floorf(_Y);

    abits = std::clamp(_abits, 1, 16);
    data_width = std::clamp(_data_width, 1, 64);
    size_t max_addr_words = size_t{1} << abits;
    size_t clamped_size = (_size > 0) ? static_cast<size_t>(_size) : max_addr_words;
    clamped_size = std::clamp<size_t>(clamped_size, 1, std::min(max_addr_words, MAX_MEM_WORDS));
    mem_size = static_cast<int>(clamped_size);

    has_write = _has_write;
    rd_sync = _rd_sync;
    wr_sync = _wr_sync;
    wr_en_width = std::clamp(_wr_en_width, 1, 64);

    mem.assign(clamped_size, 0);

    latched_rd_data = {};
    latched_rd_data.valid = true;
    latched_rd_data.type = (data_width <= 1) ? VALUE_TYPE_BOOL : VALUE_TYPE_INT;

    /* Read ports (indices 0..5). */
    add_port(-1, 1, "RD_ADDR", PORT_IN, abits);
    add_port(-1, 2, "RD_EN", PORT_IN, 1);
    add_port(-1, 3, "RD_CLK", PORT_IN, 1);
    add_port(1, sizeY, "RD_ARST", PORT_IN, 1);
    add_port(2, sizeY, "RD_SRST", PORT_IN, 1);
    add_port(sizeX, 1, "RD_DATA", PORT_OUT, data_width);

    /* Optional write ports (indices 6..9). */
    if (has_write) {
        add_port(-1, 4, "WR_ADDR", PORT_IN, abits);
        add_port(-1, 5, "WR_DATA", PORT_IN, data_width);
        add_port(-1, 6, "WR_EN", PORT_IN, wr_en_width);
        add_port(-1, 7, "WR_CLK", PORT_IN, 1);
    }

    ports[5]->update_value_final(&latched_rd_data, DEFAULT_TTL);
}

void model_mem::load_init_string(std::string_view init_str)
{
    if (init_str.empty() || mem.empty())
        return;

    std::fill(mem.begin(), mem.end(), 0);
    const uint64_t mask = width_mask(data_width);

    /*
     * Yosys/Turbostar INIT is MSB-first: bit 0 of word 0 is at the rightmost
     * character of the string.
     */
    if (init_str.size() >= 2 && init_str[0] == '0' && (init_str[1] == 'x' || init_str[1] == 'X')) {
        std::string_view hex = init_str.substr(2);
        const size_t total_bits = hex.size() * 4;
        for (size_t w = 0; w < mem.size(); ++w) {
            uint64_t word = 0;
            for (int b = 0; b < data_width; ++b) {
                const size_t bit_idx = w * static_cast<size_t>(data_width) + static_cast<size_t>(b);
                if (bit_idx >= total_bits)
                    break;
                const size_t nib_from_right = bit_idx / 4;
                const int bit_in_nib = static_cast<int>(bit_idx % 4);
                const char c = hex[hex.size() - 1 - nib_from_right];
                if ((hex_nibble(c) >> bit_in_nib) & 1)
                    word |= (uint64_t{1} << b);
            }
            mem[w] = word & mask;
        }
    } else {
        /* Binary string ('0', '1', 'x'/'X'/'z'/'Z' treated as 0). */
        const size_t total_bits = init_str.size();
        for (size_t w = 0; w < mem.size(); ++w) {
            uint64_t word = 0;
            for (int b = 0; b < data_width; ++b) {
                const size_t bit_idx = w * static_cast<size_t>(data_width) + static_cast<size_t>(b);
                if (bit_idx >= total_bits)
                    break;
                const char c = init_str[total_bits - 1 - bit_idx];
                if (c == '1')
                    word |= (uint64_t{1} << b);
            }
            mem[w] = word & mask;
        }
    }
}

uint64_t model_mem::read_word(size_t addr) const
{
    if (addr < mem.size())
        return mem[addr];
    return 0;
}

void model_mem::drawAt(class basecanvas *canvas, float _X, float _Y, int type)
{
    if (selected)
        canvas->draw_image("assets/memory_selected.png", _X, _Y, sizeX, sizeY, Alpha(type));
    else
        canvas->draw_image("assets/memory.png", _X, _Y, sizeX, sizeY, Alpha(type));

    const std::string header = has_write ? "RAM" : "ROM";
    const std::string dims = std::format("{}x{}", mem.size(), data_width);
    canvas->draw_text(header, _X + 0.3f, _Y + 0.4f, sizeX - 0.6f, 0.9f);
    canvas->draw_text(dims, _X + 0.3f, _Y + 1.5f, sizeX - 0.6f, 0.9f);

    for (auto &port : ports) {
        port->draw_wires(canvas);
        port->drawAt(canvas, _X, _Y, type);
    }
    hover_ports(canvas);
}

void model_mem::calculate(int ttl)
{
    if (ttl <= 0)
        return;

    /* Handle write port on rising clock edge (or combinational if !wr_sync). */
    if (has_write && ports.size() >= 10) {
        const bool wr_clk = ports[9]->value.boolval;
        const bool wr_rising = wr_clk && !prev_wr_clk;
        prev_wr_clk = wr_clk;

        if (!wr_sync || wr_rising) {
            const uint64_t wr_en_val = read_u64(ports[8]->value, wr_en_width);
            if (wr_en_val != 0) {
                pending_write_addr = read_u64(ports[6]->value, abits);
                pending_write_data = read_u64(ports[7]->value, data_width);
                if (wr_en_width == 1)
                    pending_write_mask = width_mask(data_width);
                else
                    pending_write_mask = wr_en_val & width_mask(data_width);
                has_pending_write = true;
                queue_calculate(this);
            }
        }
    }

    /* Handle read port (asynchronous or rising-edge synchronous). */
    const size_t rd_addr = static_cast<size_t>(read_u64(ports[0]->value, abits));
    const bool rd_en = !ports[1]->has_wires() || ports[1]->value.boolval;
    const bool rd_clk = ports[2]->value.boolval;
    const bool rd_rising = rd_clk && !prev_rd_clk;
    prev_rd_clk = rd_clk;

    if (!rd_sync) {
        const uint64_t word = read_word(rd_addr);
        struct value out = {};
        out.valid = true;
        out.type = (data_width <= 1) ? VALUE_TYPE_BOOL : VALUE_TYPE_INT;
        out.intval = word;
        out.boolval = (word != 0);
        latched_rd_data = out;
        update_value_net(&out, 5, ttl - 1);
    } else if (rd_rising && rd_en) {
        const uint64_t word = read_word(rd_addr);
        latched_rd_data.valid = true;
        latched_rd_data.type = (data_width <= 1) ? VALUE_TYPE_BOOL : VALUE_TYPE_INT;
        latched_rd_data.intval = word;
        latched_rd_data.boolval = (word != 0);
        queue_calculate(this);
    }
}

void model_mem::queued_calculate(int ttl)
{
    if (ttl <= 0)
        return;

    if (has_pending_write) {
        if (pending_write_addr < mem.size()) {
            const uint64_t mask = pending_write_mask & width_mask(data_width);
            mem[pending_write_addr] = (mem[pending_write_addr] & ~mask) |
                                      (pending_write_data & mask);
        }
        has_pending_write = false;

        /* If read is asynchronous, refresh RD_DATA after a write to the same address. */
        if (!rd_sync) {
            const size_t rd_addr = static_cast<size_t>(read_u64(ports[0]->value, abits));
            const uint64_t word = read_word(rd_addr);
            latched_rd_data.valid = true;
            latched_rd_data.type = (data_width <= 1) ? VALUE_TYPE_BOOL : VALUE_TYPE_INT;
            latched_rd_data.intval = word;
            latched_rd_data.boolval = (word != 0);
        }
    }

    update_value_net(&latched_rd_data, 5, ttl - 1);
}

void model_mem::to_json(json &j)
{
    element::to_json(j);
    j["abits"] = abits;
    j["data_width"] = data_width;
    j["mem_size"] = mem_size;
    j["has_write"] = has_write;
    j["rd_sync"] = rd_sync;
    j["wr_sync"] = wr_sync;
    j["wr_en_width"] = wr_en_width;
}

void model_mem::from_json(json &j)
{
    element::from_json(j);
    abits = std::clamp(j.value("abits", 4), 1, 16);
    data_width = std::clamp(j.value("data_width", 8), 1, 64);
    mem_size = std::clamp(j.value("mem_size", 16), 1, static_cast<int>(MAX_MEM_WORDS));
    has_write = j.value("has_write", false);
    rd_sync = j.value("rd_sync", false);
    wr_sync = j.value("wr_sync", true);
    wr_en_width = std::clamp(j.value("wr_en_width", 1), 1, 64);
    mem.assign(static_cast<size_t>(mem_size), 0);
}
