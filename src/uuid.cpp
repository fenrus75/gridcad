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

#include <random>
#include <iomanip>
#include <sstream>

std::string generate_semi_uuid(void)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    std::stringstream ss;

    ss << std::hex << std::setw(8) << std::setfill('0') << time(NULL);
    ss << "-";
    
    for (int i = 0; i < 16; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << dis(gen);
        if (i == 3 || i == 5 || i == 7 || i == 9) {
          ss << "-";
        }
    }
    
    return ss.str();
}

std::string generate_wire_name(void)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    std::stringstream ss;

    ss << "Wire-" << std::hex << std::setw(8) << std::setfill('0') << time(NULL);
    ss << "-";
    
    for (int i = 0; i < 8; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << dis(gen);
        if (i == 3 || i == 5 || i == 7 || i == 9) {
          ss << "-";
        }
    }
    
    return ss.str();
}

std::string append_random_bits(std::string input)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    std::stringstream ss;

    ss << input << std::hex << std::setw(8) << std::setfill('0');
    for (int i = 0; i < 6; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << dis(gen);
        if (i == 3 || i == 5 || i == 7 || i == 9) {
          ss << "_";
        }
    }
    
    return ss.str();
}

