#pragma once

#include <nlohmann/json.hpp>
using json = nlohmann::json;


#define DEFAULT_TTL 500
#define VALUE_TYPE_BOOL 0
#define VALUE_TYPE_INT 1
#define VALUE_TYPE_FLOAT 2
#define VALUE_TYPE_ARRAY 3
struct value {
    int type = 0;
    bool boolval = false;
    uint64_t intval = 0;
    float floatval = 0.0f;
    uint64_t arrayval = 0;
    bool is_error = false;
    bool valid = false;
    bool is_clock = false;
    bool operator==(const value &) const = default;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(value, type, boolval, intval, floatval, arrayval, is_error, valid, is_clock);
