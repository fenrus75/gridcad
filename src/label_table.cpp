/*
 * gridcad -- a tool to do digital circuit design with a GUI
 *
 * (C) Copyright 2025  -- Arjan van de Ven
 *
 * Licensed under the terms of the
 * GNU General Public LIcense version 3.0
 */

#include "label_table.h"

#include <algorithm>
#include <charconv>

namespace {

std::unordered_map<std::string, std::shared_ptr<const label_table>> &table_registry(void)
{
    static std::unordered_map<std::string, std::shared_ptr<const label_table>> reg;
    return reg;
}

} // namespace

bool label_table::add_entry(/* untrusted */ std::string_view key, /* untrusted */ std::string_view label)
{
    if (key.empty() || label.empty() || key.size() > 128)
        return false;

    /* Strip optional '_' readability separators (e.g. "b01_???_???"). */
    std::string cleaned;
    cleaned.reserve(key.size());
    for (char ch : key) {
        if (ch != '_')
            cleaned.push_back(ch);
    }
    if (cleaned.empty())
        return false;

    /* Binary pattern: "b..." or "B..." (supports '0', '1', '?' wildcards). */
    if (cleaned[0] == 'b' || cleaned[0] == 'B') {
        const std::string_view bits = std::string_view(cleaned).substr(1);
        if (bits.empty() || bits.size() > 64)
            return false;

        uint64_t mask = 0;
        uint64_t pattern = 0;
        int wildcards = 0;
        for (char c : bits) {
            mask <<= 1;
            pattern <<= 1;
            if (c == '0') {
                mask |= 1ULL;
            } else if (c == '1') {
                mask |= 1ULL;
                pattern |= 1ULL;
            } else if (c == '?') {
                ++wildcards;
            } else {
                return false;
            }
        }

        if (wildcards == 0) {
            exact_[pattern] = std::string(label);
        } else {
            wildcard_rule r;
            r.mask = mask;
            r.pattern = pattern;
            r.bit_width = static_cast<int>(bits.size());
            r.wildcard_count = wildcards;
            r.label = std::string(label);
            wildcards_.push_back(std::move(r));
        }
        raw_entries_.emplace_back(std::string(key), std::string(label));
        return true;
    }

    /* Hex ("0x..." / "0X...") or unsigned decimal exact match. */
    uint64_t val = 0;
    int base = 10;
    std::string_view num = cleaned;
    if (num.size() >= 2 && num[0] == '0' && (num[1] == 'x' || num[1] == 'X')) {
        base = 16;
        num = num.substr(2);
    }
    if (num.empty())
        return false;

    auto [ptr, ec] = std::from_chars(num.data(), num.data() + num.size(), val, base);
    if (ec != std::errc() || ptr != num.data() + num.size())
        return false;

    exact_[val] = std::string(label);
    raw_entries_.emplace_back(std::string(key), std::string(label));
    return true;
}

void label_table::finalize(void)
{
    /* Sort wildcards by fewest '?' first so the most specific pattern always wins. */
    std::stable_sort(wildcards_.begin(), wildcards_.end(),
                     [](const wildcard_rule &a, const wildcard_rule &b) {
                         return a.wildcard_count < b.wildcard_count;
                     });
}

std::string label_table::lookup(uint64_t val, int wire_width) const
{
    if (wire_width > 0 && wire_width < 64)
        val &= (uint64_t{1} << wire_width) - 1;

    /* 1. Exact matches always take precedence over wildcards. */
    auto it = exact_.find(val);
    if (it != exact_.end())
        return it->second;

    /* 2. Wildcard binary patterns (already sorted by fewest '?' first). */
    for (const auto &r : wildcards_) {
        if (r.bit_width < 64 && (val >> r.bit_width) != 0)
            continue;
        if ((val & r.mask) == r.pattern)
            return r.label;
    }

    return "";
}

std::shared_ptr<label_table> label_table::from_json(std::string_view table_name,
                                                    /* untrusted */ const nlohmann::json &j)
{
    auto tbl = std::make_shared<label_table>(table_name);
    if (!j.is_object())
        return tbl;

    for (auto it = j.begin(); it != j.end(); ++it) {
        if (it.value().is_string())
            tbl->add_entry(it.key(), it.value().get<std::string>());
    }
    tbl->finalize();
    return tbl;
}

nlohmann::json label_table::to_json(void) const
{
    nlohmann::json obj = nlohmann::json::object();
    for (const auto &[k, v] : raw_entries_)
        obj[k] = v;
    return obj;
}

void label_table::register_table(const std::shared_ptr<const label_table> &tbl)
{
    if (!tbl || tbl->get_name().empty())
        return;
    table_registry()[tbl->get_name()] = tbl;
}

std::shared_ptr<const label_table> label_table::find_registered(std::string_view table_name)
{
    if (table_name.empty())
        return nullptr;
    auto &reg = table_registry();
    auto it = reg.find(std::string(table_name));
    if (it != reg.end())
        return it->second;
    return nullptr;
}
