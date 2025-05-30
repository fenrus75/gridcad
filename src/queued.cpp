#include "gridcad.h"

#include "value.h"

static std::vector<class element *> queue0, queue1;
static int queue = 0;

void queue_calculate(class element *element)
{
    if (queue == 0) {
        queue0.push_back(element);
    } else {
        queue1.push_back(element);
    }
        
}

void run_queued_calculations(void)
{
//    printf("Running calculation queue with sizes %li %li \n", queue0.size(), queue1.size());
    if (queue == 0) {
        for (auto elem : queue0) 
            elem->queued_calculate(DEFAULT_TTL);
        queue0.clear();
        queue = 1;
    } else {
        for (auto elem : queue1) 
            elem->queued_calculate(DEFAULT_TTL);
        queue1.clear();
        queue = 0;
    }
}

void remove_from_calculate_queue(class element *element)
{
    bool restart = false;
    do {
        restart = false;
        for (unsigned int i = 0; i < queue0.size(); i++) {
            if (queue0[i] == element) {
                restart = true;
                queue0.erase(queue0.begin() + i);
                break;
            }
        }
        for (unsigned int i = 0; i < queue1.size(); i++) {
            if (queue1[i] == element) {
                restart = true;
                queue1.erase(queue1.begin() + i);
                break;
            }
        }
    } while (restart);
}