#pragma once
#include "gridcad.h"



class autocomplete_element {
public:
    autocomplete_element(void) {};
    ~autocomplete_element(void) {};
    class element *from = NULL, *to = NULL;
    class port *from_port = NULL, *to_port = NULL;
    
    class wire *tempwire = NULL;
};

/* gui canvas to draw on */
class canvas : public basecanvas
{
public:
    canvas (class scene *_scene, struct capabilities *cap = NULL);
    virtual ~canvas(void);
    
    void draw(void) override;
    
    class scene *get_scene(void) override { return current_scene; };
    class scene *swap_scene(class scene *scene);
    bool handle_event(SDL_Event &event) override;
    

    void to_json(json& j);
    class scene *get_undo(void);
    void take_undo_snapshot(class scene *scene);
    void from_json_to_floating(json &j);
    
    void update_window_title(void);
    float distance_from_mouse(float X, float Y) override;

    void clear_floating(void);
    
    float screen_width(void) override;
    
    void set_dialog(class dialog *D) { dialogbox = D;};
    void set_project_name(std::string _projectname) { projectname = _projectname;};
    
         
protected:
    bool draw_grid = false;
    bool window_shown = true;
    class scene *current_scene = NULL;
    SDL_Rect main_area_rect, ui_area_rect, button_rect;
    class element *dragging = NULL;
    std::vector<class element *> floating;
    class port *dragging_port = NULL;
    class wire *dragging_wire = NULL, *hover_wire = NULL;
    class iconbar *icon_bar = NULL;
    class icon *active_icon = NULL;
    class buttonbar *button_bar = NULL;
    class contextmenu *active_menu = NULL;
    class dialog *dialogbox = NULL;
    bool left_mouse_down = false;
    float mouseX = 0.0, mouseY = 0.0;
    uint64_t mouse_timestamp = 0; /* SDL_GetTicks64 time of when the mouse last moved */
    float click_start_X = 0.0, click_start_Y = 0.0;
    int middle_X = 0, middle_Y = 0;
    bool shift_down = false; /* not saved in json */
    std::vector<std::string> undo_list;
    bool in_area_select = false;
    float area_select_X1 = 0.0, area_select_Y1 = 0.0;
    SDL_Texture *area_select_texture = NULL;
    bool tooltip_eligable = false;
    std::string projectname = "";
    
    
    bool handle_event_iconarea(SDL_Event &event);
    bool handle_event_drawingarea(SDL_Event &event);
    void draw_tooltip(float X, float Y, std::string tooltip);
    bool fittoscreen = false;
    float DPI = 96;    
    
    bool show_toolchain = false;
    
    void zap_autocomplete(void);
    void create_autocomplete_from_wire(class port *port1, class port *port2);
    void draw_autocomplete(void);
    void apply_autocomplete(void);
    
    std::vector<struct autocomplete_element *> autocomplete;
};
