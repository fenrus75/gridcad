#pragma once

#include "gridcad.h"

class barbutton;

class buttonbar
{
public:
    buttonbar(class canvas *canvas, unsigned int width);
    ~buttonbar(void);
    
    void draw_at(class canvas *canvas, int W, int H);    
    void handle_event(SDL_Event &event);
    
    void add_button(std::string text, std::string icon, int event);
protected:
    class canvas *canvas = NULL;
    unsigned int width = 0;
    
    std::vector<class barbutton *>buttons;
};


class barbutton
{
public:
    barbutton(std::string text, std::string icon, int event, class canvas *canvas);
    ~barbutton(void);

    void draw_at(class canvas *canvas, float X, float Y, float  W, float H);    
    
    void activate(void);
    void deactivate(void) {active = false; };

protected:

    class canvas *canvas = NULL;    
    std::string text = "";
    std::string icon = "";
    int eventnr = 0;
    bool active = false;
};
