#pragma once
#include "gridcad.h"
#include "value.h"
#define PORT_IN 0
#define PORT_OUT 1
#define PORT_INOUT 2

class port : public base
{
public:
    port(std::string _name, int _direction = PORT_IN, int bus_width = 1);    
    virtual ~port(void);
    virtual std::string class_id(void) { return "port:";};
    int X, Y;
    float screenX, screenY;
    std::string name = "";
    class element *parent = NULL;
    struct value value = {};
    void add_wire(class wire *wire);
    void update_value(struct value *newvalue, int ttl);
    int direction = PORT_IN;;
    
    void draw(class canvas *canvas, int color);
    void draw_wires(class canvas *canva);
    void drawAt(class canvas *canvas, float X, float Y, int color);
    void stop_drag(class canvas *canva);
    virtual void notify(int ttl);
    class wire *is_wire(float X, float Y);
    void fill_grid(class wiregrid *grid);
    
    void remove_wire(class wire *wire);
    bool replace_wire(class wire *from, class wire *to);
    void to_json(json &j);
    void from_json(json &j);
    bool has_wires(void) { return wires.size() > 0; };
    void remove_wires(void);
    void remove_orphans(void);
    int get_bus_width(void) { return bus_width; };
    void update_name(std::string newname);
private:
    int bus_width = 1;
    void drawConnector(class canvas *canvas, float X, float Y, int cX, int cY, int type);    
    std::vector<class wire*> wires;
};
