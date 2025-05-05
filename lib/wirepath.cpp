#include "wirepath.h"

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
    best_path = 4.0 * width * height;
}


void wiregrid::debug_display(void)
{
    int x,y;
    
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
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
    assert(x < width);
    assert(y < height);
    grid[y][x].blocked = true;
};

/*
 * For the path finding algorithm we need a lower bound estimate
 * of the remaining distance. Traditionally one uses the pythagoras distance,
 * but for our case, since we only allow 0, 45 and 90 degree lines,
 * it's much simpler.
 */
double wiregrid::cost_estimate(int x, int y)
{
    int max, min;
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

static int recursecount = 0;

bool wiregrid::one_path_walk(double cost_so_far, int x, int y, int dx, int dy)
{
    double costs[9];
    bool walked[9];
    double leastcost = cost_so_far + 4 * width * height;
    double maxcost = 0;
    int i;
    
    recursecount++;
    
    if (x < 0 || x >= width)
        return false;
    if (y < 0 || y >= height)
        return false;
        
    if (x == targetX && y == targetY) {
        /* we found the end point */
        if (cost_so_far < best_path) {
            best_path = cost_so_far;
            grid[y][x].dirX = -dx;
            grid[y][x].dirY = -dy;
        }
        return true;     
    }
    
    if (cost_so_far > best_path)
        return false;
        
    if (grid[y][x].blocked)
        return false;

    /* we've been to this cell before with lower cost than the current path */
    if (grid[y][x].valid and grid[y][x].distance <= cost_so_far)
        return false;
        
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
        costs[i] += COST[i];
        
        /* don't go straight back */
        if (DX[i] == -dx && DY[i] == -dy)
            walked[i] = true;
        
        if (costs[i] > best_path)
            walked[i] = true;
        if (costs[i] > maxcost)
            maxcost = costs[i];
        if (costs[i] < leastcost)
            leastcost = costs[i];
    }

    if (maxcost > best_path)
        maxcost = best_path;

    walked[4] = true;
    while (leastcost < maxcost+0.31) {
        for (i = 0; i < 9; i++) {
             if (walked[i])
                 continue;
             if (costs[i] <= leastcost) {
                 one_path_walk(cost_so_far + COST[i], x + DX[i], y + DY[i], DX[i], DY[i]);
                 walked[i] = true;
             }   
        }
    
        leastcost += 0.3;
    }

    return false;    
}

void wiregrid::walk_back(void)
{
    int x, y;
    
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
        dx =  grid[y][x].dirX;
        dy = grid[y][x].dirY;
        x += dx;
        y += dy;
    }
}

void wiregrid::path_walk(int x1, int y1, int x2, int y2)
{
    originX = x1;
    originY = y1;
    targetX = x2;
    targetY = y2;
    
    one_path_walk(0.0, x1, y1, 0, 0);
    walk_back();
    printf("Final best cost %5.2f \n", best_path);
    printf("Recursion count %i\n", recursecount);
}