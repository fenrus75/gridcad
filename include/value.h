#pragma once

#include <nlohmann/json.hpp>
using json = nlohmann::json;


#define DEFAULT_TTL 500
#define VALUE_TYPE_BOOL 0
#define VALUE_TYPE_INT 1
#define VALUE_TYPE_FLOAT 2
#define VALUE_TYPE_ARRAY 3
struct value {
    int type;
    bool boolval;
    uint64_t intval;
    float floatval;
    uint64_t arrayval;
    bool is_error;
    bool valid;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(value, type, boolval, intval, floatval, arrayval, is_error, valid);
