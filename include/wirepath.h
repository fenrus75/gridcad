#pragma once

#include <vector>

struct point {
    bool valid;
    bool blocked;
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
    int width, height;
};
