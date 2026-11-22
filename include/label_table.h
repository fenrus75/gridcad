/*
 * gridcad -- a tool to do digital circuit design with a GUI
 *
 * (C) Copyright 2025  -- Arjan van de Ven
 *
 * Licensed under the terms of the
 * GNU General Public LIcense version 3.0
 */

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

/*
 * Shared value-to-label lookup table for Verilog enums and opcode/bit-pattern tables.
 *
 * Precedence:
 *   1. Exact matches (hex "0x...", decimal, or binary "b0101..." with no '?' wildcards).
 *   2. Wildcard binary patterns ("b01??????"), sorted by fewest '?' wildcards first.
 */
class label_table
{
public:
    struct wildcard_rule {
        uint64_t mask = 0;
        uint64_t pattern = 0;
        int bit_width = 0;
        int wildcard_count = 0;
        std::string label = "";
    };

    label_table(void) = default;
    explicit label_table(std::string_view table_name) : name_(table_name) {}

    const std::string &get_name(void) const noexcept { return name_; }
    void set_name(std::string_view table_name) { name_ = std::string(table_name); }

    bool empty(void) const noexcept { return exact_.empty() && wildcards_.empty(); }
    size_t size(void) const noexcept { return exact_.size() + wildcards_.size(); }

    /* Parse and insert a single untrusted key ("0x0f", "b00??0001", "12") -> label entry. */
    bool add_entry(/* untrusted */ std::string_view key, /* untrusted */ std::string_view label);

    /* Finalize wildcard ordering (fewest '?' wildcards first). Called automatically by from_json(). */
    void finalize(void);

    /* Look up a value, masking to wire_width bits when wire_width is in [1, 63]. */
    std::string lookup(uint64_t val, int wire_width = 0) const;

    /* Parse from a JSON object { "<key>": "<label>", ... }. */
    static std::shared_ptr<label_table> from_json(std::string_view table_name,
                                                  /* untrusted */ const nlohmann::json &j);
    nlohmann::json to_json(void) const;

    /* Central process-wide registry so wires & Undo snapshots reference tables by name. */
    static void register_table(const std::shared_ptr<const label_table> &tbl);
    static std::shared_ptr<const label_table> find_registered(std::string_view table_name);

private:
    std::string name_ = "";
    std::unordered_map<uint64_t, std::string> exact_;
    std::vector<wildcard_rule> wildcards_;
    std::vector<std::pair<std::string, std::string>> raw_entries_;
};
