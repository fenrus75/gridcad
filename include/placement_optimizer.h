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
#include <string>

class scene;
class element;

/*
 * Incremental geometric placement optimizer for gridcad schematics.
 *
 * Evaluates a fast port-to-port cost function (Manhattan wirelength, straight
 * horizontal/vertical alignment bonus via bend penalty, backward-edge penalty,
 * and high locality weight for unpinned constants) and performs step-by-step
 * 8-direction hill-climbing moves.
 */
class placement_optimizer
{
public:
    struct step_result {
        bool moved = false;
        int steps_taken = 0;
        std::string moved_element;
        int dx = 0;
        int dy = 0;
        double cost_before = 0.0;
        double cost_after = 0.0;
    };

    /* Compute total geometric placement cost for a scene. */
    static double compute_scene_cost(class scene *target_scene);

    /* Compute cost contribution of wires incident to a single element. */
    static double compute_element_cost(class element *elem);

    /*
     * Run one shuffled 8-direction (+ jump/swap + 1% opposite-move escape)
     * optimization sweep over every movable element in the scene.
     */
    static step_result optimize_all_elements(class scene *target_scene,
                                             int max_follow_steps = 5);
};

/* Tracks whether gridcad is running in imported-Verilog view mode (for UI button behavior). */
bool is_import_view_mode(void);
void set_import_view_mode(bool enabled);
