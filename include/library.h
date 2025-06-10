#pragma once
#include "gridcad.h"


struct library_block {
    std::string name;
    std::string collection;
    std::string logic; /* json format */
    std::string tooltip; 
    
    std::string icon; /* file path */
    std::string icon_selected; /* file path */
};

extern std::vector<struct library_block> library;
extern struct library_block * find_in_library(std::string collection, std::string element);
