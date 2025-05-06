#include "wirepath.h"
#include <stdio.h>

int main(int argc, char **argv)
{
    int y;
    class wiregrid * grid = new wiregrid(50, 33);
    std::vector<struct waypoint> *points;
    
    for (y = 20 ; y < 30; y++)
        grid->block_point(20, y);
        
    points = grid->path_walk(15,25,25,25);
    
    grid->debug_display();
    delete grid;
    
    for (auto p: *points) {
        printf("X, Y : %i, %i\n", p.X, p.Y);
    }
    return 0;
}