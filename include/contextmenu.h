#pragma once

#include "gridcad.h"


typedef void (*callback_fn)(class element *element);
typedef void (*scene_callback_fn)(class scene *scene);
typedef void (*port_callback_fn)(class port *port, int color);

struct context_item {
    std::string menu_text;
    int color;
    callback_fn callback; 
    scene_callback_fn scene_callback; 
    port_callback_fn port_callback; 
};

class contextmenu {
public:
    contextmenu(class element *element);
    contextmenu(class scene *scene);
    ~contextmenu(void);

    void add_item(std::string text, callback_fn callback);    
    void add_item(std::string text, scene_callback_fn callback);    
    
    virtual void draw_at(class basecanvas * canvas, float X, float Y);
    void draw_at(class basecanvas * canvas);
    
    void mouse_motion(float X, float Y);
    void mouse_set(float X, float Y);
    virtual void mouse_click(float X, float Y);
    
    bool mouse_in_bounds(float X, float Y);
    
protected:
    class element *element = NULL;
    class scene *scene = NULL;
    std::vector<struct context_item *> items;
    
    int selection = -1;
    
    float X1 = 0, Y1 = 0, X2 = 0, Y2 = 0;
    float maxX = 0, maxY = 0;

    float currentX = 0, currentY = 0;
    
    int textheight = 0;
};

class port_contextmenu : public contextmenu {
public:
    port_contextmenu(class port *port);
    ~port_contextmenu(void);

    void add_item(std::string text, int color, port_callback_fn callback);    
    
    void draw_at(class basecanvas * canvas, float X, float Y) override;
    void mouse_click(float X, float Y) override;

private:
    class port *port = NULL;
    std::vector<struct context_item *> items;   
};
