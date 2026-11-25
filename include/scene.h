#pragma once
#include "gridcad.h"
#include <unordered_map>

/* logical representation of a (sub)circuit */
class scene : public base
{
public:
    scene(std::string name, std::string parent = "");
    virtual ~scene(void);
    
    std::string class_id(void) override { return "scene:";};

    float sizeX = 200, sizeY = 200;
    
    void add_element(std::unique_ptr<class element> element);
    void remove_element(class element *element);
    
    bool can_place_element(float x, float y, int w, int h, class element *myself = nullptr);

    class port * is_port(float X, float Y); /* X and Y are global positions */
    std::shared_ptr<class wire> is_wire(float X, float Y);
    
    void fill_grid(class wiregrid* grid);
    
    void deselect_all(void);
    class element *selected_element(void);
    
    void to_json(json& j);
    void selection_to_json(json& j);
    void from_json(json& j);
    void process_delete_requests(void);
    void delete_selection(void);

    /* needs to become protected */
    std::vector<std::unique_ptr<class element>> elements;
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
    class contextmenu *get_menu(void) { return menu.get(); };
    void cycle_color(void);
    void reroute_all_wires(void);
    
    std::string get_verilog_main(void);
    std::string get_verilog_modules(std::string verilog_path);
    std::string get_verilog_name(void) { return verilog_name;};
    void create_verilog_names(void);

    void remove_nets(void);
    void add_nets(void);
    void sync_nets(void);
    void redo_nets(void);
    void redo_net(class element *seed_elem);
    void redo_all_multi_sink_nets(void);
    void set_needs_steiner_routing(bool needed) { needs_steiner_routing = needed; }
    void maybe_run_steiner_routing(void) {
        if (needs_steiner_routing) {
            needs_steiner_routing = false;
            redo_all_multi_sink_nets();
        }
    }
    void set_layout_project_key(std::string_view key) { layout_project_key = std::string(key); }
    const std::string &get_layout_project_key(void) const { return layout_project_key; }
    void set_layout_module_name(std::string_view mod) { layout_module_name = std::string(mod); }
    const std::string &get_layout_module_name(void) const {
        return layout_module_name.empty() ? name : layout_module_name;
    }
    void mark_layout_dirty(void) { layout_dirty = true; }
    void clear_layout_dirty(void) { layout_dirty = false; }
    bool is_layout_dirty(void) const { return layout_dirty; }
    void validate_ports(void);

    size_t broadcast_vcd_value(std::string_view vcd_name, const struct value &val);
    size_t broadcast_vcd_values(const std::unordered_map<std::string, struct value> &values);
    void unlock_all_vcd(void);
    void collect_vcd_names(std::vector<std::string> &out_names) const;
    void collect_vcd_qualifiers(std::unordered_map<std::string, std::pair<std::string, bool>> &out_qualifiers) const;
    const std::vector<std::string> &get_pinned_waves(void) const noexcept { return pinned_waves; }
    void set_pinned_waves(std::vector<std::string> waves) { pinned_waves = std::move(waves); }
    
protected:
    std::unique_ptr<class contextmenu> menu;
    std::string name = "";
    std::string vname = ""; /* nosave */
    std::string parental_name = "";    
    std::string verilog_name = "";
    uint64_t generation_count = 0;
    bool has_sequencer = false;
    bool needs_steiner_routing = false;
    std::string layout_project_key = "";
    std::string layout_module_name = "";
    bool layout_dirty = false;
    std::vector<std::string> pinned_waves;
};

