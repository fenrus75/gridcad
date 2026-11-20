#pragma once
#include "gridcad.h"
#include "wirepath.h"
#include "value.h"


class net;

class wire : public base, public std::enable_shared_from_this<wire>
{
public:
    wire(int x1, int y1, int x2, int y2, int _color = 0);
    virtual std::string class_id(void) { return "wire:";};
    
    void move_target(int x2, int y2);
    void draw(class basecanvas *, int color = COLOR_WIRE_SOLID);
    void route(class scene *);
    
    void add_port(class port *port);
    void del_port(class port *port);
    void reseat(void);
    
    void update_value_final(struct value *newvalue, int ttl);
    void update_value_net(struct value *newvalue, int ttl);
    virtual void notify(int ttl);
    struct value value = {};
    bool intersect(float targetX, float targetY);
    
    void select(void) { selected = true;};
    void deselect(void) { selected = false;};
    void remove(void);
    void remove_if_orphan(void);
    void fill_grid(class wiregrid* grid);
    void check_reverse(void);
    void clear_route(void);
    
    std::shared_ptr<class wire> split(void);
    
    std::string name = "";
    
    std::string get_verilog_name(void);
    
    void to_json(json &j);
    void from_json(json &j);
    void set_new_name(void);
    
    bool is_empty(void) { return ports.size() == 0; };
    const std::vector<class port *> &get_ports(void) const { return ports; };
    void redo_wires(void);
    int get_width(void) { return width; };
    void set_width(int _width) { if (_width) width = _width;};
    class port * get_other_port(class port *port);
    void push_wire_color(int color);
    void set_color(int _color) {color = _color;} ;
    int get_color(void) { return color; };
    
    std::string get_verilog_decl(void);
    
    void clear_distance_from_outport(void) { distance_from_outport = INT_MAX - 5;};
    unsigned int get_distance_from_outport(void) { return distance_from_outport;};
    int set_distance_from_outport(unsigned int dist) { if (dist < distance_from_outport) {distance_from_outport = dist; return 1;} return 0;};
    bool update_distances(void);

    void add_net(class net *net);
    void remove_net(void);
    bool has_net(void) { return net != nullptr;};
    class net *get_net(void) { return net ; };
    void set_is_z(bool z) { is_z  = z; };
    bool get_is_z(void) { return is_z; };

    void set_vcd_name(std::string_view name) { vcd_name = std::string(name); };
    const std::string &get_vcd_name(void) const { return vcd_name; };
    bool is_vcd_locked(void) const { return vcd_locked; };
    void unlock_vcd(void) { vcd_locked = false; };
    void set_vcd_value_direct(const struct value &v);
    void apply_vcd_value(const struct value &v);

    void set_label_table(std::shared_ptr<const std::map<uint64_t, std::string>> tbl) { label_table = std::move(tbl); };
    const std::shared_ptr<const std::map<uint64_t, std::string>> &get_label_table(void) const { return label_table; };
    std::string get_label(uint64_t val) const;

    void set_src_info(std::string_view loc, std::string_view verilog) {
        src_location = std::string(loc);
        src_verilog = std::string(verilog);
    }
    const std::string &get_src_location(void) const { return src_location; }
    const std::string &get_src_verilog(void) const { return src_verilog; }
    
    void calculate_drawpoints(void);
    
protected:
    bool dead = false;
    bool selected = false;
    bool being_routed = false; /* not saved in json */
    unsigned int distance_from_outport = INT_MAX;
    std::vector<class port *> ports;
    int X1 = 0, Y1 = 0, X2 = 0, Y2 = 0;
    int color = 0;
    int width = 0;
    std::unique_ptr<std::vector<struct waypoint>> points;
    std::unique_ptr<std::vector<struct waypoint>> drawpoints;
    std::unique_ptr<std::vector<struct waypoint>> oldpoints;
    void add_midpoint(float x1, float y1, float x2, float y2, float effect);
    class net *net = nullptr; /* nosave */
    bool is_reversed = false;
    bool is_z = false; /* nosave -- net recalculates this */
    bool in_animation = false; /* nosave */
    uint64_t points_timestamp = 0; /* nosave */
    std::string vcd_name = ""; /* hierarchical VCD signal name */
    bool vcd_locked = false;   /* true while driven/locked by VCD playback */
    std::shared_ptr<const std::map<uint64_t, std::string>> label_table;
    std::string src_location = ""; /* imported Verilog file:line (nosave) */
    std::string src_verilog = "";  /* imported Verilog source snippet (nosave) */
};

