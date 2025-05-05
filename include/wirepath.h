#pragma once

#include <vector>

/* approximationm == slightly above actual sqrt(2) to favor non-diagnals */
#define SQRT2 1.45

struct point {
    bool valid;
    bool blocked;
    bool part_of_wire;
    double distance;
    
    double extra_score;
};

class wiregrid {
public:
    wiregrid(int width, int height);

    void debug_display(void);    
    
    void block_point(int x, int y);
private:
    std::vector<std::vector<struct point>> grid;
    
    double cost_estimate(int x, int y);
    int width, height;
    
    int originX, originY;
    int targetX, targetY;
};
