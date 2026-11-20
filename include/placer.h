/*
 * gridcad -- a tool to do digital circuit design with a GUI
 *
 * (C) Copyright 2025  -- Arjan van de Ven
 *
 * Licensed under the terms of the
 * GNU General Public LIcense version 3.0
 */

#pragma once

#include <cstddef>

class scene;

/*
 * Initial topological left-to-right placer for imported netlists.
 *
 * Assigns elements to columns based on combinational depth (breaking sequential
 * feedback loops at register outputs), orders elements within each column using
 * a barycenter heuristic to reduce wire crossings, and sizes the scene grid
 * with generous routing margins.
 */
class topological_placer
{
public:
    struct options {
        int margin_x = 4;
        int margin_y = 4;
        int col_gap = 6;
        int row_gap = 3;
    };

    static void place(class scene *target_scene);
    static void place(class scene *target_scene, const options &opts);
};
