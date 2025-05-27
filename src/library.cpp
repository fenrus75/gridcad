#include "gridcad.h"
#include "iconbar.h"

#include "library.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <fstream>


std::vector<struct library_block> library;



void populate_library(std::string directory)
{
    const std::filesystem::path libpath{directory};
    
    for (auto const &dir_entry : std::filesystem::directory_iterator{libpath}) {
        std::string path = dir_entry.path();
        
        if (path.ends_with(".json")) {
            struct library_block block;
            block.name = dir_entry.path().filename();
	    block.name = block.name.substr(0, block.name.size()-5);
            std::ifstream input(path);
            
            std::string line = "";
            
            while (std::getline(input, line)) {
                block.logic += line;
            }
            block.icon = path + ".png";
            block.icon_selected = path + ".selected.png";
            library.push_back(block);
        }
    }
}


