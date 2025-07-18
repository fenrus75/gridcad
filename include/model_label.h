#pragma once

#include "gridcad.h"
#include "name.h"

class model_label : public element
{
public:
    model_label(float X, float Y);
    virtual ~model_label(void);
    virtual std::string class_id(void) { return "model_label:";};


    virtual void to_json(json &j) override;
    virtual void from_json(json &j) override;

    void handle_event(class basecanvas *canvas, SDL_Event &event) override;
    void draw_early(class basecanvas *canvas, int type) override;
    void draw(class basecanvas *canvas, int type) override {};
    void drawAt(class basecanvas*, float, float, int) override {};
    void start_drag(float X, float Y) override;
    void update_drag(class basecanvas *canvas, class scene *scene,  float X, float Y) override;
    bool stop_drag(class basecanvas *canva) override;
    void fill_grid(class wiregrid* grid) override {};

    
    bool in_edit_mode(void) override { return name_edit->get_edit_mode() && single && selected;};
    void enter_edit_mode(void) override { name_edit->set_edit_mode(true); selected = true; single = true;};    
    std::string get_verilog_main(void) override { return "";};
    void set_color(int c) { color = c; };
    int get_color(void) { return color; };
    void rotate_ports(void) override;
    
    bool is_background(void) override {return true;};
private:
    class name *name_edit = NULL;
    void drawAt_early(class basecanvas*, float, float, int);
    int color = 0;
    bool bottomlabel = true;
    bool drag_is_resize = false;
};

