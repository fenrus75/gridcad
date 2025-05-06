#pragma once

#include <vector>

/* approximationm == slightly above actual sqrt(2) to favor non-diagnals */
#define SQRT2 1.45

struct point {
    bool valid;
    bool blocked;
    bool part_of_wire;
    bool visited;
    double distance;
    
    int dirX, dirY; /* direction we xame from to get best cost to get here */
    int dir_to_goal;
    
    double extra_score;
};

struct waypoint {
    int X, Y;
};

class wiregrid {
public:
    wiregrid(int width, int height);
    ~wiregrid();

    void debug_display(void);    
    
    void block_point(int x, int y);
    void unblock_point(int x, int y);
    void add_soft_cost(int x, int y, double extra);
    bool is_blocked(int x, int y);
    
    std::vector<struct waypoint> *  path_walk(int x1, int y1, int x2, int y2);
private:
    std::vector<std::vector<struct point>> grid;
    
    double best_path;
    int width, height;

    int originX, originY;
    int targetX, targetY;
    
    double cost_estimate(int x, int y);
    
    std::vector<struct waypoint> *  walk_back(void);
    
    bool one_path_walk(double cost_so_far, int x, int y, int dx, int dy, int recurse);
};
