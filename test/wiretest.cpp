#include "wirepath.h"


int main(int argc, char **argv)
{
    int y;
    class wiregrid * grid = new wiregrid(50, 33);
    
    for (y = 20 ; y < 30; y++)
        grid->block_point(20, y);
    
    grid->debug_display();
    return 0;
}