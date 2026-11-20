#pragma once
#include "gridcad.h"



class timelinebar;
class vcd_session;

class autocomplete_element {
public:
    autocomplete_element(void) = default;
    ~autocomplete_element(void);
    class element *from = nullptr, *to = nullptr;
    class port *from_port = nullptr, *to_port = nullptr;
    
    std::shared_ptr<class wire> tempwire;
};

/* gui canvas to draw on */
class canvas : public basecanvas
{
public:
    canvas (std::unique_ptr<class scene> _scene, struct capabilities *cap = nullptr);
    virtual ~canvas(void);
    
    void draw(void) override;
    
    class scene *get_scene(void) override { return current_scene.get(); };
    std::unique_ptr<class scene> swap_scene(std::unique_ptr<class scene> scene);
    bool handle_event(SDL_Event &event) override;
    

    void to_json(json& j);
    std::unique_ptr<class scene> get_undo(void);
    void take_undo_snapshot(class scene *scene);
    void from_json_to_floating(json &j);
    
    void update_window_title(void);
    float distance_from_mouse(float X, float Y) override;

    void clear_floating(void);
    
    float screen_width(void) override;
    
    void set_dialog(std::unique_ptr<class dialog> D);
    void set_project_name(std::string _projectname) { projectname = _projectname;};
    void set_vcd_session(class vcd_session *sess);
    
         
protected:
    bool draw_grid = false;
    bool window_shown = true;
    std::unique_ptr<class scene> current_scene;
    SDL_Rect main_area_rect, ui_area_rect, button_rect;
    class element *dragging = nullptr;
    std::vector<std::unique_ptr<class element>> floating;
    class port *dragging_port = nullptr;
    std::shared_ptr<class wire> dragging_wire;
    std::weak_ptr<class wire> hover_wire;
    std::unique_ptr<class iconbar> icon_bar;
    class icon *active_icon = nullptr;
    std::unique_ptr<class buttonbar> button_bar;
    std::unique_ptr<class timelinebar> timeline_bar;
    class contextmenu *active_menu = nullptr;
    std::unique_ptr<class dialog> dialogbox;
    bool left_mouse_down = false;
    float mouseX = 0.0, mouseY = 0.0;
    uint64_t mouse_timestamp = 0; /* SDL_GetTicks64 time of when the mouse last moved */
    float click_start_X = 0.0, click_start_Y = 0.0;
    int middle_X = 0, middle_Y = 0;
    bool shift_down = false; /* not saved in json */
    std::vector<std::string> undo_list;
    bool in_area_select = false;
    float area_select_X1 = 0.0, area_select_Y1 = 0.0;
    sdl::TexturePtr area_select_texture;
    bool tooltip_eligable = false;
    std::string projectname = "";
    
    
    bool handle_event_iconarea(SDL_Event &event);
    bool handle_event_drawingarea(SDL_Event &event);
    void draw_tooltip(float X, float Y, std::string_view tooltip);
    bool fittoscreen = false;
    float DPI = 96;    
    
    bool show_toolchain = false;
    bool auto_optimize = false;
    uint64_t last_optimize_ms = 0;
    void run_optimize_phase(void);
    
    void zap_autocomplete(void);
    void create_autocomplete_from_wire(class port *port1, class port *port2);
    void draw_autocomplete(void);
    void apply_autocomplete(void);
    void best_port_to_autocomplete(class port *origin);
    
    std::vector<std::unique_ptr<class autocomplete_element>> autocomplete;
};
