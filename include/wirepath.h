#pragma once

#include <memory>
#include <queue>
#include <vector>
#include <nlohmann/json.hpp>
/* approximationm == slightly above actual sqrt(2) to favor non-diagnals */
#define SQRT2 1.45

struct point {
    int x = 0;
    int y = 0;
    bool valid = false;
    bool blocked = false;
    bool part_of_wire = false;
    bool visited = false;
    double distance_from_start = 0.0;
    double hamdist = 0.0;
    
    int dirX = 0, dirY = 0; /* direction we came from to get best cost to get here */
    int dir_to_goal = 0;
    
    bool on_path = false;
    bool queued = false;
    
    double extra_score = 0.0;
};

struct waypoint {
    float X = 0.0f, Y = 0.0f;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(waypoint, X, Y);

struct CompareTask {
    bool operator()(const struct point * t1, const struct point * t2) {
        return t1->distance_from_start + t1->hamdist > t2->distance_from_start + t2->hamdist;
    }
};

class wiregrid {
public:
    wiregrid(int width, int height);
    void debug_display(int X = 0, int Y=0);    
    
    void block_point(int x, int y);
    void declare_part_of_wire(int x, int y);
    void unblock_point(int x, int y);
    void add_soft_cost(int x, int y, double extra);
    bool is_blocked(int x, int y);
    float get_soft_cost(int x, int y);
    
    struct seed_point {
        int x = 0;
        int y = 0;
        double initial_cost = 0.0;
    };

    std::unique_ptr<std::vector<struct waypoint>> path_walk(int x1, int y1, int x2, int y2, bool is_clock = false);
    /* Multi-source A-star/Dijkstra: finds the optimal seed in sources and path to (x2, y2) in one walk. */
    std::unique_ptr<std::vector<struct waypoint>> path_walk_multi(const std::vector<seed_point> &sources,
                                                                  int x2, int y2, bool is_clock = false);

    bool is_clock = false;
private:
    std::vector<std::vector<struct point>> grid;

    bool found_solution = false;    
    double best_path = 500000;
    int width = 0, height = 0;

    int originX = 0, originY = 0;
    int targetX = 0, targetY = 0;
    

    double cost_estimate(int x, int y);
    void update_destination(int tX, int tY);
    std::unique_ptr<std::vector<struct waypoint>> walk_back(void);
    
    void visit(struct point *p);
    
    std::priority_queue<struct point *, std::vector<struct point *>, CompareTask> queue;
    
};
