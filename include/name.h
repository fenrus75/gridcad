#pragma once


#include "gridcad.h"

class name
{
public:
    name(std::string * value);
    ~name(void);
    
    
    void handle_event(SDL_Event &event);

    void drawAt(class canvas *canvas, float X, float Y, float W);
    
    void set_edit_mode(bool em) { edit_mode = em; };
    void toggle_edit_mode(void) { edit_mode = !edit_mode; printf("TOGGLE%i\n", edit_mode);};
    bool get_edit_mode(void) { return edit_mode;};


private:

    void insert_char(unsigned char c);
    
    std::string * value = NULL;
    unsigned int cursorpos = 0;
    bool edit_mode = false;
};