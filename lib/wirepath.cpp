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
}


void wiregrid::debug_display(void)
{
    int x,y;
    
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            if (grid[y][x].blocked) {
                printf("X");
                continue;
            }
            printf(".");
        }
        printf("\n");
    }
}

void wiregrid::block_point(int x, int y) 
{
    assert(x < width);
    assert(y < height);
    grid[y][x].blocked = true;
};

double wiregrid::cost_estimate(int x, int y)
{
    int max, min;
    if (x > y) {
        max = x;
        min = y;
    } else {
        max = y;
        min = x;
    }
    return SQRT2 * min + (max - min);
}