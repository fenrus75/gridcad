#include "gridcad.h"
#include "element.h"
#include "value.h"

#include <algorithm>

static std::vector<class element *> queue0, queue1;
static int queue = 0;

void queue_calculate(class element *element)
{
    auto &target = (queue == 0) ? queue0 : queue1;
    if (std::find(target.begin(), target.end(), element) == target.end())
        target.push_back(element);
}

void run_queued_calculations(void)
{
    /*
     * Swap into a local batch and flip the active queue BEFORE running
     * queued_calculate(), so any elements queued during calculation go into
     * the other buffer and never invalidate iterators on the running batch.
     */
    for (int pass = 0; pass < 2; ++pass) {
        std::vector<class element *> batch;
        if (queue == 0) {
            batch.swap(queue0);
            queue = 1;
        } else {
            batch.swap(queue1);
            queue = 0;
        }
        for (auto *elem : batch)
            elem->queued_calculate(DEFAULT_TTL);
    }
}

void remove_from_calculate_queue(class element *element)
{
    std::erase(queue0, element);
    std::erase(queue1, element);
}
