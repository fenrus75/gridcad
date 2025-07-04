#pragma once
#include "gridcad.h"

/* logical representation of a (sub)circuit */
class scene : public base
{
public:
    scene(std::string name, std::string parent = "");
    virtual ~scene(void);
    
    virtual std::string class_id(void) { return "scene:";};

    float sizeX = 200, sizeY = 200;
    
    void add_element(class element *element);
    void remove_element(class element *element);
    
    bool can_place_element(float x, float y, int w, int h, class element *myself = NULL);

    class port * is_port(float X, float Y); /* X and Y are global positions */
    class wire *is_wire(float X, float Y);
    
    void fill_grid(class wiregrid* grid);
    
    void deselect_all(void);
    class element *selected_element(void);
    
    void to_json(json& j);
    void selection_to_json(json& j);
    void from_json(json& j);
    void process_delete_requests(void);
    void delete_selection(void);

    /* needs to become protected */
    std::vector<class element *> elements;
    void rewire_section(int x1, int y1, int w, int h);
    void rewire_section(class element *element);
    void remove_orphans(void);

    unsigned int selected_count(void);
    
    std::string get_name(void) { return name; };
    std::string get_full_name(void);
    void set_parental_name(std::string name) { parental_name = name;};
    void update_name(std::string _name) { name = _name;};
    void update_vname(std::string _name) { vname = _name;};
    uint64_t get_generation_count(void) { return generation_count; };
    class contextmenu *get_menu(void) { return menu; };
    void cycle_color(void);
    void reroute_all_wires(void);
    
    std::string get_verilog_main(void);
    std::string get_verilog_modules(std::string verilog_path);
    std::string get_verilog_name(void) { return verilog_name;};
    void create_verilog_names(void);

    void remove_nets(void);
    void add_nets(void);
    void redo_nets(void);
    void validate_ports(void);
    
protected:
    class contextmenu *menu = NULL;
    std::string name = "";
    std::string vname = ""; /* nosave */
    std::string parental_name = "";    
    std::string verilog_name = "";
    uint64_t generation_count = 0;
    bool has_sequencer = false;
};

