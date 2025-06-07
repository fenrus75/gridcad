#pragma once
#include "gridcad.h"
#include "wirepath.h"
#include "value.h"

class wire : public base
{
public:
    wire(int x1, int y1, int x2, int y2, int _color = 0);
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
    struct value value = {};
    bool intersect(float targetX, float targetY);
    
    void select(void) { selected = true;};
    void deselect(void) { selected = false;};
    void remove(void);
    void remove_if_orphan(void);
    void fill_grid(class wiregrid* grid);
    
    class wire *split(void);
    
    std::string name = "";
    
    std::string get_verilog_name(void);
    
    void to_json(json &j);
    void from_json(json &j);
    void set_new_name(void);
    
    bool is_empty(void) { return ports.size() == 0; };
    void redo_wires(void) { delete points; points = NULL; };
    int get_width(void) { return width; };
    void set_width(int _width) { if (_width) width = _width;};
    class port * get_other_port(class port *port);
    void push_wire_color(int color);
    int get_color(void) { return color; };
    
    std::string get_verilog_decl(void);
    
protected:
    bool dead = false;
    bool selected = false;
    bool being_routed = false; /* not saved in json */
    std::vector<class port *> ports;
    int X1 = 0, Y1 = 0, X2 = 0, Y2 = 0;
    int color = 0;
    int refcount = 0;
    int width = 0;
    std::vector<struct waypoint> *points = NULL;
};

