#pragma once

#include "gridcad.h"

class nestcanvas;

class model_nest : public element
{
public:
    model_nest(float X, float Y);
    virtual ~model_nest(void);
    virtual std::string class_id(void) { return "model_nest:";};


    void drawAt(class canvas *canvas, float X, float Y, int type) override;
    bool mouse_select(float X, float Y) override;
/*
    virtual void to_json(json &j) override;
    virtual void from_json(json &j) override;
    */
    void calculate(int ttl) override;
    void handle_event(SDL_Event&) override;
    bool in_edit_mode(void) override { return edit_mode && single && selected;};    
private:
    class canvas *canvas = NULL;
    long int previous_click = -1; 
    class scene *_scene = NULL;
    bool edit_mode = NULL;
};
