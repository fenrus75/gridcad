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

#include "wirepath.h"
#include <algorithm>

#include <stdio.h>
#include <assert.h>

wiregrid::wiregrid(int _width, int _height)
{
    int y;
    width = _width;
    height = _height;    
    grid.resize(_height);
    for (y = 0; y < _height; y++)
        grid[y].resize(width);
        
    /* start with an upper bound on best path */
    best_path = 3.0 * width * height;
    found_solution = false;
}

wiregrid::~wiregrid()
{
    int y;
    for (y = 0; y < height; y++)
        grid[y].resize(0);
    grid.resize(0);
    
}


void wiregrid::debug_display(int _X, int _Y)
{
    int x,y;
    
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            if (x == originX && y == originY) {
                printf("O");
                continue;
            }
            if (x == targetX && y == targetY) {
                printf("T");
                continue;
            }
            
            if (grid[y][x].dir_to_goal >=0 && grid[y][x].valid) {
                printf("G");
                continue;
            }
            
            if (x == _X && y == _Y) {
                printf("&");
                continue;
            }
            if (grid[y][x].blocked) {
                printf("|");
                continue;
            }
            #if 1
            if (grid[y][x].part_of_wire) {
                printf("=");
                continue;
            }
            #endif
            if (grid[y][x].dirX && grid[y][x].dirY) {
                printf("/");
                continue;
            }
            if (grid[y][x].dirX) {
                printf("x");
                continue;
            }
            if (grid[y][x].dirY) {
                printf("y");
                continue;
            }
            
            if (grid[y][x].valid) {
                printf("*");
                continue;
            }
            printf(".");
        }
        printf("\n");
    }
    printf("\n");
}

void wiregrid::block_point(int x, int y) 
{
    if (x < 0 || x >= width)
        return;
    if (y < 0 || y >= height)
        return;
    grid[y][x].blocked = true;
};
void wiregrid::declare_part_of_wire(int x, int y) 
{
    if (x < 0 || x >= width)
        return;
    if (y < 0 || y >= height)
        return;
    grid[y][x].part_of_wire = true;
};
void wiregrid::unblock_point(int x, int y) 
{
    if (x < 0 || x >= width)
        return;
    if (y < 0 || y >= height)
        return;
    grid[y][x].blocked =  false;
};

bool wiregrid::is_blocked(int x, int y)
{
    if (x < 0 || x >= width)
        return true;
    if (y < 0 || y >= height)
        return true;
    return grid[y][x].blocked;
}

float wiregrid::get_soft_cost(int x, int y)
{
    if (x < 0 || x >= width)
        return 0;
    if (y < 0 || y >= height)
        return 0;
    return grid[y][x].extra_score;
}

void wiregrid::add_soft_cost(int x, int y, double extra)
{
    if (x < 0 || x >= width)
        return;
    if (y < 0 || y >= height)
        return;
    if (extra >= 0)
	    grid[y][x].extra_score += extra;
    else
	    grid[y][x].extra_score = std::min(extra, grid[y][x].extra_score);

    /* negative values are allowed to encourage parallel routing etc -- but need to
       be clipped to some relatively small amount or the algorithm becomes unstable
       as very long routings can become cheap and loops etc will get encouraged */
    if (grid[y][x].extra_score < -0.1)
	grid[y][x].extra_score = -0.1;
}

/*
 * For the path finding algorithm we need a lower bound estimate
 * of the remaining distance. Traditionally one uses the pythagoras distance,
 * but for our case, since we only allow 0, 45 and 90 degree lines,
 * it's much simpler.
 */
double wiregrid::cost_estimate(int x, int y)
{
    int max, min;
    
    if (is_blocked(x, y) || x <0 || y < 0)
        return 4.0 * width * height;
        
    x -= targetX;
    y -= targetY;
    if (x < 0)
        x = -x;
    if (y < 0)
        y = -y;
    if (x > y) {
        max = x;
        min = y;
    } else {
        max = y;
        min = x;
    }
    return SQRT2 * min + (max - min);
}

/* 
 * 0 1 2
 * 3 4 5
 * 6 7 8
 */
static const int DX[9] = {-1, 0, 1, -1, 0, 1, -1, 0, 1};
static const int DY[9] = {-1, -1, -1, 0, 0, 0, 1, 1, 1};
static const float COST[9] = {SQRT2, 1, SQRT2, 1, 0, 1, SQRT2, 1, SQRT2 };
static const float COST2[9] = {1.4 * SQRT2, 1, 1.4 * SQRT2, 1, 0, 1, 1.4 * SQRT2, 1, 1.4 * SQRT2 };

static int recursecount = 0;

static bool abort_all_walks = false;

bool wiregrid::one_path_walk(double cost_so_far, int x, int y, int dx, int dy, int recurse, bool is_clock)
{
    double costs[9];
    bool walked[9];
    double leastcost = cost_so_far + 4 * width * height;
    double maxcost = 0;
    int i;
    const float *EFFECTIVE_COST;
    
    if (abort_all_walks)
        return false;

    EFFECTIVE_COST = COST;
    if (is_clock) {
	EFFECTIVE_COST = COST2; /* we want clocks to be routed with far fewer diagonals */
    }
    
    recursecount++;
 
 //   printf("Best %5.2f   current %5.2f\n", best_path, cost_so_far);
//     debug_display(x, y);   
    
    if (x < 0 || x >= width)
        return false;
    if (y < 0 || y >= height)
        return false;
        
    if (x == targetX && y == targetY) {
        found_solution = true;
        /* we found the end point */
        if (cost_so_far < best_path) {
            best_path = cost_so_far;
            grid[y][x].dirX = -dx;
            grid[y][x].dirY = -dy;
        }
        return true;     
    }
    
    cost_so_far += grid[y][x].extra_score;
    
    if (cost_so_far > best_path)
        return false;
        
    if (grid[y][x].blocked) 
        return false;
        
//    if (grid[y][x].dir_to_goal < 0 && grid[y][x].visited)
//        return false;

    /* we've been to this cell before with lower cost than the current path */
    if (grid[y][x].valid and grid[y][x].distance <= cost_so_far)
        return false;

    if (recurse > 1000) {
        printf("Recursion limit hit\n");
        abort_all_walks = true;
        return false;
    }
    if (!grid[y][x].valid)
        grid[y][x].dir_to_goal = -1;
        
    grid[y][x].valid = true;
    grid[y][x].distance = cost_so_far;        
    grid[y][x].dirX = -dx;
    grid[y][x].dirY = -dy;

    for (i = 0; i < 9; i++)
        walked[i] = 0;
        
    for (i = 0; i < 9 ; i++) {
        if (i == 4)
            continue;
        costs[i] = cost_estimate(x + DX[i], y + DY[i]);
        costs[i] += EFFECTIVE_COST[i];
        
        /* don't go straight back */
        if (DX[i] == -dx && DY[i] == -dy)
            walked[i] = true;
        
        if (costs[i] > best_path)
            walked[i] = true;
        if (costs[i] > maxcost)
            maxcost = costs[i];
        if (costs[i] < leastcost)
            leastcost = costs[i];
        if (costs[i] > best_path)
            walked[i] = true;
    }

    if (maxcost > best_path)
        maxcost = best_path;

    walked[4] = true;

    /* if we know where the goal is -- lets go there first to quickly update path distance */    
    if (grid[y][x].dir_to_goal >= 0) {
        double adder = 0.0;
        
        i = grid[y][x].dir_to_goal;
        if (DX[i] != dx || DY[i] != dy)
             adder += 0.01;
        one_path_walk(cost_so_far + EFFECTIVE_COST[i] + adder, x + DX[i], y + DY[i], DX[i], DY[i], recurse+1, is_clock);
        walked[i] = true;
    }
    bool ret = false;
    while (leastcost < maxcost+0.3) {
        for (i = 0; i < 9; i++) {
             double adder = 0.0;
             if (walked[i])
                 continue;
             if (costs[i] <= leastcost) {
                 bool this_ret;
                 
                 /* pay a small penalty for direction changes to give same length paths with fewer changes a bonus*/
                 if (DX[i] != dx || DY[i] != dy) {
                     adder += 0.1;
		     if (is_clock)
			adder += 0.4;
		 }
                 this_ret = one_path_walk(cost_so_far + EFFECTIVE_COST[i] + adder, x + DX[i], y + DY[i], DX[i], DY[i], recurse+1, is_clock);
                 ret |= this_ret;
                 if (ret)
                     grid[y][x].dir_to_goal = i;
                 walked[i] = true;
             }   
        }
        leastcost += 0.3;
    }
    grid[y][x].visited = true;   

    return ret;    
}

std::vector<struct waypoint> *  wiregrid::walk_back(void)
{
    int x, y;
    int prevDX = 0, prevDY = 0;
    
    std::vector<struct waypoint> *vec;
    
    vec = new std::vector<struct waypoint>;
    struct waypoint wp;
    
    if (abort_all_walks) {
        wp.X = targetX;
        wp.Y = targetY;
        vec->push_back(wp);
        wp.X = originX;
        wp.Y = originY;
        vec->push_back(wp);
        std::reverse(vec->begin(), vec->end());
        abort_all_walks = false;
        return vec;
    }
    
    x = targetX;
    y = targetY;

    while (true) {
        int dx, dy;
        if (grid[y][x].dirX == 0 && grid[y][x].dirY == 0)
            break;
        grid[y][x].part_of_wire = true;
        if (x == originX && y == originY) {
            break;
        }
        dx = grid[y][x].dirX;
        dy = grid[y][x].dirY;
        
        if (dx != prevDX || dy != prevDY) {
            /* we're changing direction */
            wp.X = x;
            wp.Y = y;
            vec->push_back(wp);
        }
        
        prevDX = dx;
        prevDY = dy;
        x += dx;
        y += dy;
        if (x < 0 || x >= width)
            break;
        if (y <0 || y >= height)
            break;
    }
    wp.X = x;
    wp.Y = y;
    vec->push_back(wp);
    std::reverse(vec->begin(), vec->end());
    return vec;
}

std::vector<struct waypoint> * wiregrid::path_walk(int x1, int y1, int x2, int y2, bool is_clock)
{
    if (x1 < 0)
        x1 = 0;
    if (x1 >= width)
        x1 = width -1;
    if (y1 < 0)
        y1 = 0;
    if (y1 >= height)
        y1 = height - 1;
    if (x2 < 0)
        x2 = 0;
    if (x2 >= width)
        x2 = width -1;
    if (y2 < 0)
        y2 = 0;
    if (y2 >= height)
        y2 = height - 1;
    originX = x1;
    originY = y1;
    targetX = x2;
    targetY = y2;
    
    recursecount = 0;
 
    abort_all_walks = false;   
    
    /* make sure the origin is not blocked -- which is quite possible as by default all terminal nodes
     * are blocked */
    unblock_point(x1, y1);   
    unblock_point(x2, y2);   
    one_path_walk(0.0, x1, y1, 0, 0, 0, is_clock);
    printf("recursecount is %i \n", recursecount);
 //   debug_display();
    return walk_back();
}