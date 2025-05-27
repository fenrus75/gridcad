#pragma once
#include "gridcad.h"
#include "iconbar.h"


struct library_block {
    std::string name;
    std::string logic; /* json format */
    
    std::string icon; /* file path */
};

extern std::vector<struct library_block> library;
