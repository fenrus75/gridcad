#pragma once
#include "gridcad.h"


class element : public base
{
public:    
    element(int sizeX, int sizeY, std::string _name, std::string _parent = "");
    virtual ~element(void);

    virtual std::string class_id(void) { return "element:";};
    
    void place(int X, int Y);
    
    virtual void drawAt(class basecanvas *canvas, float X, float Y, int type);
    virtual void draw_early(class basecanvas *canvas, int type) {};
    virtual void draw(class basecanvas *canvas, int type);
    virtual void draw_phase2(class basecanvas *canvas, int type);
    
    virtual void start_drag(float X, float Y);
    virtual void update_drag(class basecanvas *canvas, class scene *scene,  float X, float Y);
    virtual bool stop_drag(class basecanvas *canva);
    
    virtual bool intersect(float X, float Y);
    virtual bool intersect_full(float X, float Y);  /* includes the ports */
    virtual bool intersect_float(float X, float Y);  /* includes the ports */
    
    const std::string  get_name(void) { return name; };
    const std::string  get_full_name(void) { return parental_name + "/" + name; };
    
    void add_port(int X, int Y, const char *name, int direction = 0, int _width = 0);
    virtual void fill_grid(class wiregrid* grid);
    
    class port * is_port(float X, float Y); /* X and Y are global positions */
    
    float get_X(void) { return X; };
    float get_Y(void) { return Y; };
    float get_width(void) { return sizeX; };
    float get_height(void) { return sizeY; };
    
    void update_value(class port *port, struct value *value, int ttl);
    virtual void notify(int ttl);
    virtual void calculate(int ttl);
    virtual void queued_calculate(int ttl) {};
    virtual bool mouse_select(float X, float Y); /* scene absolute */
    bool has_moved(void) { return over_drag_threshold;};
    class wire *is_wire(float X, float Y);
    
    void reseat(void);
    virtual void to_json(json& j);
    virtual void from_json(json& j);
    void select(void) { selected = true; single = false; reseat();};
    void select_single(void) { selected = true; single = true; reseat();};
    void deselect(void) { selected = false; single = false;};
    bool is_selected(void) { return selected;};
    virtual bool want_deleted(void);
    void delete_if_selected(void);
    void remove_orphans(void);
    
    virtual bool in_edit_mode(void) { return false; };
    
    virtual void handle_event(class basecanvas *canvas, SDL_Event &event);
    
    std::string get_uuid(void) { return uuid;};
    void reset_uuid(void);
    void delete_element(void);
    
    virtual void rotate_ports(void);
    void update_name(std::string _name) { name = _name;};
    virtual void update_parental_name(std::string _name) { parental_name = _name;};
    std::string get_parental_name(void) { return parental_name;};
    void hover_ports(class basecanvas *canvas);
    virtual class port *get_clk_port(void) { return nullptr;};
    void connect_clk(class port *clk);
    virtual void enter_edit_mode(void) {};
    virtual void reset_pointer(void) {};
    
    class contextmenu *get_menu(void) { return menu;};
    
    void cycle_color(void);
    void reroute_all_wires(void);
    virtual void create_verilog_name(int seqno, std::vector<std::string> *existing);
    virtual std::string get_verilog_name(void) { return verilog_name;};
    virtual std::string get_verilog_width(void) { return "";};
    virtual std::string get_verilog_main(void) { return "Element " + class_id() + " not implemented\n";};
    virtual std::string get_verilog_modules(std::string verilog_path) { return "";};
    void collect_nets(std::vector<std::string> *wiremap);
    virtual void set_library_origin(std::string origin_lib, std::string origin_elm) {};
    virtual bool is_background(void) { return false; };

    void remove_nets(void);
    void free_nets_memory(void);
    void add_nets(void);

    void update_value_net(struct value *value, int port, int ttl);
    int has_port(class port *port); /* true if this element has this port */
    class port * port_at(unsigned int i) { if (i < ports.size()) return ports[i]; else return NULL; };
    void validate_ports(void);
    
protected:
    class contextmenu *menu = NULL; /* nosave */
    std::string uuid = "";
    std::string name = "";
    std::string parental_name = "";    
    std::string verilog_name = "";
    int sizeX = 1;
    int sizeY = 1;
    int angle = 0;
    bool over_drag_threshold = false;  /* has a drag-and-drop been far enough to avoid spurious drags */
    bool want_delete = false; /* not saved to disk */
    
    
    float X = 0, Y = 0;
    float Xghost = 0, Yghost = 0;
    float Xdnd = 0, Ydnd = 0;
    float X_in_drag = 0, Y_in_drag = 0;
    bool selected = false; /* not saved to disk */
    bool single = false; /* only selected item in the scene - not saved to disk */
    float mouseX = -100, mouseY = -100; /* nosave */
    
    std::vector<class port *> ports;
    std::vector<class net *> my_nets; /* nets allocated by this element */
    std::vector<class wire *> my_wires; /* wires allocated by this element */
    
};


