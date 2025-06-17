#pragma once
#include "gridcad.h"
#include "value.h"
#include "contextmenu.h"
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
    int value_ttl = 0;
    void add_wire(class wire *wire);
    void update_value(struct value *newvalue, int ttl);
    int direction = PORT_IN;
    bool is_connector = false;
    int color = 0;
    
    void draw(class canvas *canvas, int color);
    void draw_wires(class canvas *canva);
    void drawAt(class canvas *canvas, float X, float Y, int color);
    void drawAt2(class canvas *canvas, float X, float Y, int color);
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
    int get_width(void);
    void set_width(int _width) { bus_width = _width;};
    void update_name(std::string newname);
    void link_uuid(std::string _uuid);
    std::string get_linked_uuid(void) { return linked_uuid; };
    void set_linked_element(class element* elem) { linked_element = elem; };
    class element *get_linked_element(void) { return linked_element; };
    void route_wires();
    void unsplice(void);
    void push_wire_color(int color);
    class contextmenu *get_menu(void) { return menu;};
    void add_menu(void);
    
    void cycle_color(void);
    void reroute_all_wires(void);
    void collect_wires(std::map<std::string, std::string> *wiremap);
    void collect_wires(std::vector<std::string> *wiremap);
    void collect_wires(std::map<std::string, unsigned int> *wiremap);
    
    std::string get_verilog_name(void);
    
    unsigned int get_distance_from_outport(void) { return distance_from_outport;};
    void update_distances(void);

	
private:
    class port_contextmenu *menu = NULL;
    int bus_width = 0;
    void drawConnector(class canvas *canvas, float X, float Y, int cX, int cY, int type);    
    std::vector<class wire*> wires;
    std::string linked_uuid = "";
    class element *linked_element = NULL;
    std::string verilog_name = "";
    unsigned int distance_from_outport = INT_MAX;
};
