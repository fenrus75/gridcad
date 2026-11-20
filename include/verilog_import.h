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
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

class scene;

/*
 * Imports a Turbostar structural Verilog JSON netlist into a gridcad scene
 * hierarchy and runs topological placement.
 */
class verilog_importer
{
public:
    struct options {
        bool use_clock_element = true;  /* Use model_clock for top-level "clk" port */
        bool run_placer = true;         /* Run topological_placer after building netlist */
        bool load_sibling_files = true; /* Resolve missing submodules from <dir>/<type>.json */
        int max_recursion_depth = 32;
    };

    struct stats {
        size_t ports = 0;
        size_t constants = 0;
        size_t components = 0;
        size_t submodules = 0;
        size_t wires = 0;
        size_t unsupported_components = 0;
        size_t missing_endpoints = 0;
        std::vector<std::string> warnings;
    };

    struct result {
        bool ok = false;
        std::string error;
        std::string top_module;
        std::unique_ptr<class scene> imported_scene;
        std::map<std::string, std::map<uint64_t, std::string>> label_tables;
        struct stats stats;
    };

    static result import_file(std::string_view json_path);
    static result import_file(std::string_view json_path, const options &opts);

    static result import_json(const nlohmann::json &root,
                              const std::filesystem::path &base_dir = {});
    static result import_json(const nlohmann::json &root,
                              const options &opts,
                              const std::filesystem::path &base_dir = {});
};
