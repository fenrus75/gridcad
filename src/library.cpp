/*
 * gridcad -- a tool to do digital circuit design with a GUI
 *
 * (C) Copyright 2025  -- Arjan van de Ven
 *
 * Licensed under the terms of the
 * GNU General Public LIcense version 3.0
 *
 * Authors:
 *    Arjan van de Ven <arjanvandeven@gmail.com>
 */

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
    
    for (auto const &dir_entry : std::filesystem::recursive_directory_iterator{libpath}) {
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

            std::ifstream input_tooltip(path + ".tooltip");
            
            line = "";
            
            while (std::getline(input_tooltip, line)) {
                block.tooltip += line;
            }

            library.push_back(block);
        }
    }
}


