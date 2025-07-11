#pragma once
#include "gridcad.h"
#include "value.h"
#include "contextmenu.h"
#define PORT_IN 0
#define PORT_OUT 1
#define PORT_INOUT 2
#define PORT_Z 3

class net;

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
    void update_value_final(struct value *newvalue, int ttl);
    void update_value_net(struct value *newvalue, int ttl);
    int direction = PORT_IN;
    bool is_connector = false;
    int color = 0;
    
    void draw(class basecanvas *canvas, int color);
    void draw_wires(class basecanvas *canva);
    void drawAt(class basecanvas *canvas, float X, float Y, int color);
    void drawAt2(class basecanvas *canvas, float X, float Y, int color);
    void drawDistress(class basecanvas *canvas, float X, float Y);
    void stop_drag(class basecanvas *canva);
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
    int get_net_width(void);
    void set_width(int _width) { bus_width = _width;};
    void update_name(std::string newname);
    void link_uuid(std::string _uuid);
    std::string get_linked_uuid(void) { return linked_uuid; };
    void set_linked_element(class element* elem) { linked_element = elem; };
    class element *get_linked_element(void) { return linked_element; };
    void route_wires();
    void unsplice(void);
    void push_wire_color(int color);
    void push_net_color(int color);
    class contextmenu *get_menu(void) { return menu;};
    void add_menu(void);
    
    void cycle_color(void);
    void reroute_all_wires(void);
    void collect_nets(std::vector<std::string> *netmap);
    void validate(void);
    void set_distress(void) { distress = true; };
    void clear_distress(void) { distress = false; };
    
    std::string get_verilog_name(void);
    std::string get_net_verilog_name(std::string fallback = ""); /* verilog name of the net connected */
    std::string get_net_verilog_wire_decl(void); /* verilog full wire line of the net connected */
    
    unsigned int get_distance_from_outport(void) { if (direction == PORT_OUT) return 0; else return distance_from_outport;};
    bool update_distances(void);
    
    std::string get_tooltip(void);

    void add_net(class net *net);
    void remove_net(void);

    bool has_net(void);
    class net * get_net(void);
    void check_reverse(void);
    std::string get_verilog_width(void);
    void allow_fancy_verilog(void) { allow_fancy_verilog_name = true; };
    void update_fancy_name(std::string name);
	
private:
    class port_contextmenu *menu = NULL;
    int bus_width = 0;
    void drawConnector(class basecanvas *canvas, float X, float Y, int cX, int cY, int type);    
    std::vector<class wire*> wires;
    std::string linked_uuid = "";
    class element *linked_element = NULL;
    std::string verilog_name = "";
    unsigned int distance_from_outport = INT_MAX;
    bool distress = false;
    bool allow_fancy_verilog_name = false;
};
