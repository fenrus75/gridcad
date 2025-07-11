#pragma once

#include "gridcad.h"

class barbutton;

class buttonbar
{
public:
    buttonbar(class basecanvas *canvas, unsigned int width);
    ~buttonbar(void);
    
    void draw_at(class basecanvas *canvas, int W, int H);    
    void handle_event(SDL_Event &event);
    
    void add_button(std::string text, std::string icon, int event);
    
    std::string current_tooltip(unsigned int X, unsigned int Y);
    void set_fps(float _fps) { fps = _fps; };
protected:
    class basecanvas *canvas = NULL;
    unsigned int width = 0;
    unsigned int mouseX = 0;
    unsigned int mouseY = 0;
    unsigned int hover_button = 8888;
    uint64_t hover_time = 0;
    float fps = -1.0;
    
    std::vector<class barbutton *>buttons;
};


class barbutton
{
public:
    barbutton(std::string text, std::string icon, int event, class basecanvas *canvas);
    ~barbutton(void);

    void draw_at(class basecanvas *canvas, float X, float Y, float  W, float H, bool hover = false, float delta = 0.0);    
    
    void activate(void);
    void deactivate(void) {active = false; };
    std::string get_tool_tip(void) { return text;};

protected:

    class basecanvas *canvas = NULL;    
    std::string text = "";
    std::string icon = "";
    int eventnr = 0;
    bool active = false;
};
