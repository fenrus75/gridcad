#pragma once

#include "gridcad.h"
#include "name.h"
class nestcanvas;

class model_nest : public element
{
public:
    model_nest(float X, float Y);
    virtual ~model_nest(void);
    virtual std::string class_id(void) { return "model_nest:";};


    void drawAt(class canvas *canvas, float X, float Y, int type) override;
    bool mouse_select(float X, float Y) override;

    void to_json(json &j) override;
    void from_json(json &j) override;

    void calculate(int ttl) override;
    void handle_event(class canvas *canvas, SDL_Event&) override;
    bool in_edit_mode(void) override { return name_edit->get_edit_mode() && single && selected;};    
    void regen_ports(void);
    
    void enter_edit_mode(void) override { name_edit->set_edit_mode(true); selected = true; single = true;};
    
    void load_scene_from_json(std::string);
    void set_icon(std::string _icon, std::string _selected) { icon = _icon; icon_selected = _selected;};
private:

    float bX1,bY1,bX2,bY2; /* bounding box of the scene -- no save*/
    class scene *_scene = NULL;
    class canvas *canvas = NULL;
    long int previous_click = -1;  /* no save */
    std::vector<class port *> north, south, east, west; /* nosave */
    void place_port(class port *port);
    void maybe_regen_ports(void);
    uint64_t previous_generation_count = 0;
    std::string icon = "";
    std::string icon_selected = "";
    class name *name_edit = NULL;
};
