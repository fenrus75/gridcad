#pragma once


#include "gridcad.h"

class name
{
public:
    name(std::string * value);
    ~name(void);
    
    
    void handle_event(SDL_Event &event);

    void drawAt(class basecanvas *canvas, float X, float Y, float W, float H = 1.0);
    
    void set_edit_mode(bool em) { edit_mode = em; };
    void toggle_edit_mode(void) { edit_mode = !edit_mode; printf("TOGGLE%i\n", edit_mode);};
    bool get_edit_mode(void) { return edit_mode;};
    void want_numbers_only(void) { numbers_only = true; };
    void allow_spaces(void) { spaces = true; };
    
    void update_string_pointer(std::string *value);



private:

    void insert_char(unsigned char c);
    void backspace(void);
    float relative_cursor_pos(class basecanvas *canvas, std::string text, float H = 1.0);
    
    std::string * value = NULL;
    unsigned int cursorpos = 0;
    bool edit_mode = false;
    bool numbers_only = false;
    bool spaces = false;
};

