#pragma once

#include "gridcad.h"


typedef void (*callback_fn)(class element *element);

struct context_item {
    std::string menu_text;
    callback_fn callback; /* FIXME: function pointer type */
};

class contextmenu {
public:
    contextmenu(class element *element);
    ~contextmenu(void);

    void add_item(std::string text, callback_fn callback);    
    
    void draw_at(class basecanvas * canvas, float X, float Y);
    void draw_at(class basecanvas * canvas);
    
    void mouse_motion(float X, float Y);
    void mouse_click(float X, float Y);
    
    bool mouse_in_bounds(float X, float Y);
    
private:
    class element *element;
    std::vector<struct context_item *> items;
    
    int selection = -1;
    
    float X1 = 0, Y1 = 0, X2 = 0, Y2 = 0;
    float maxX = 0, maxY = 0;
    
    int textheight = 0;
};