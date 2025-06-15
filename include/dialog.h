#pragma once

#include "gridcad.h"


/* modal dialog box */

class dialog {
public:
    dialog(int screenX, int screenY);
    ~dialog(void);
    
    void draw(class basecanvas *canvas);
    virtual void handle_event(SDL_Event &event);
    
    void update_screen_size(int _screenX, int _screenY) { screenX = _screenX; screenY = _screenY; };
    
    
    void append_line(std::string line) { content.push_back(line); };
    void replace_content(std::vector<std::string> newcontent) { content = newcontent; };
    void enable_ok_button(void) { has_ok_button = true; };
private:
    int screenX = 0;
    int screenY = 0;
    std::vector<std::string> content;
    bool has_ok_button = false;
};