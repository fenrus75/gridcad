#pragma once
#include "gridcad.h"
#include "wirepath.h"
#include "value.h"

class wire : public base
{
public:
    wire(int x1, int y1, int x2, int y2, int _color = COLOR_WIRE_SOLID);
    virtual ~wire(void);

    virtual std::string class_id(void) { return "wire:";};
    
    void move_target(int x2, int y2);
    void draw(class canvas *, int color = COLOR_WIRE_SOLID);
    void route(class scene *);
    
    void get_ref(void);
    void add_port(class port *port);
    void reseat(void);
    
    void update_value(struct value *newvalue, int ttl);
    virtual void notify(int ttl);
    struct value value;
    bool intersect(float targetX, float targetY);
    
    void select(void) { selected = true;};
    void deselect(void) { selected = false;};
    void remove(void);
    void remove_if_orphan(void);
    void fill_grid(class wiregrid* grid);
    
    class wire *split(void);
    
    std::string name = "";
    
    void to_json(json &j);
    void from_json(json &j);
    void set_new_name(void);
    
    bool is_empty(void) { return ports.size() == 0; };
    
protected:
    bool selected = false;
    bool being_routed = false; /* not saved in json */
    std::vector<class port *> ports;
    int X1 = 0, Y1 = 0, X2 = 0, Y2 = 0;
    int color = 0;
    int refcount = 0;
    std::vector<struct waypoint> *points = NULL;
};

