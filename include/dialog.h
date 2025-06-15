#pragma once

#include "gridcad.h"


/* modal dialog box */

class dialog {
public:
    dialog(int screenX, int screenY, std::string minwidthstring = "");
    virtual ~dialog(void);
    
    void draw(class basecanvas *canvas);
    virtual void handle_event(class canvas *canvas, SDL_Event &event);
    
    void update_screen_size(int _screenX, int _screenY) { screenX = _screenX; screenY = _screenY; };
    
    
    void append_line(std::string line) { content.push_back(line); };
    void replace_content(std::vector<std::string> newcontent) { content = newcontent; };
    void enable_ok_button(void) { has_ok_button = true; };
protected:
    int screenX = 0;
    int screenY = 0;
    std::string minwtext = "";
    std::vector<std::string> content;
    bool has_ok_button = false;
    
    float buttonX1 = 0, buttonY1 = 0,buttonW = 0, buttonH = 0;
    float currentX = 0, currentY = 0;
    float largestX = 0;
    int largest_lines = 0;
};