/*
 * gridcad -- a tool to do digital circuit design with a GUI
 *
 * (C) Copyright 2025  -- Arjan van de Ven
 *
 * Licensed under the terms of the
 * GNU General Public LIcense version 3.0
 */

#pragma once

#include <filesystem>
#include <string>
#include <string_view>

class scene;

/*
 * Persistent per-module layout sidecar for imported Verilog netlists.
 *
 * Stores user edits (element positions/rotations, wire colors, area labels,
 * and data scopes with their attached target nets — never runtime signal
 * values) under:
 *   ~/.cache/gridcad/project_<hash>/layouts/<module_name>.json
 *
 * Only scenes marked layout_dirty are written, so unedited modules always
 * receive fresh default P&R.
 */
class layout_overlay
{
public:
    /* Resolve ~/.cache/gridcad/project_<hash>/layouts/ for a project key. */
    static std::filesystem::path layout_dir_for_project(std::string_view project_key);
    static std::filesystem::path layout_file_for_module(std::string_view project_key,
                                                        std::string_view module_name);

    /* Save a single scene's layout sidecar to an explicit file path (also used by unit tests). */
    static bool save_scene_to_file(class scene *target_scene, const std::filesystem::path &path);
    /* Apply a saved layout sidecar from an explicit file path onto a freshly placed scene. */
    static bool apply_file_to_scene(class scene *target_scene, const std::filesystem::path &path);

    /* Save all layout_dirty scenes in a hierarchy (root + submodules) to their cache paths. */
    static size_t save_dirty_recursive(class scene *root_scene);

    /* Apply cached layout for (project_key, target_scene->get_name()) if one exists. */
    static bool apply_to_scene(class scene *target_scene, std::string_view project_key);

    /* Delete cached layout file for target_scene (if any) and clear its dirty flag. */
    static bool reset_scene_layout(class scene *target_scene);
};
