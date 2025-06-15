#pragma once

#include "gridcad.h"


typedef void (*callback_fn)(class element *element);
typedef void (*scene_callback_fn)(class scene *scene);
typedef void (*port_callback_fn)(class port *port, int color);
typedef void (*icon_callback_fn)(class iconbar *bar, class oneiconbar *choice);

class oneiconbar;

struct context_item {
    std::string menu_text;
    int color;
    class oneiconbar *choice;
    callback_fn callback; 
    scene_callback_fn scene_callback; 
    port_callback_fn port_callback; 
    icon_callback_fn icon_callback; 
    bool active;
};

class contextmenu {
public:
    contextmenu(class element *element);
    contextmenu(class scene *scene);
    virtual ~contextmenu(void);

    void add_item(std::string text, callback_fn callback);    
    void add_item(std::string text, scene_callback_fn callback);    
    
    virtual void draw_at(class basecanvas * canvas, float X, float Y);
    virtual void draw_at(class basecanvas * canvas);
    
    void mouse_motion(float X, float Y);
    void mouse_set(float X, float Y);
    virtual void mouse_click(float X, float Y);
    
    bool mouse_in_bounds(float X, float Y);
    
    void set_active(std::string);
    void set_inactive(std::string);
    
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
    virtual ~port_contextmenu(void);

    void add_item(std::string text, int color, port_callback_fn callback);    
    
    void draw_at(class basecanvas * canvas, float X, float Y) override;
    void mouse_click(float X, float Y) override;

private:
    class port *port = NULL;
};

class oneiconbar;

class icon_contextmenu : public contextmenu {
public:
    icon_contextmenu(class iconbar *bar);
    virtual ~icon_contextmenu(void);

    void add_item(std::string text, class oneiconbar*, icon_callback_fn callback);    
    void draw_at(class basecanvas * canvas, float X, float Y) override;
    void draw_at(class basecanvas * canvas) override;
    
    void mouse_click(float X, float Y) override;

private:
    class iconbar *bar = NULL;
};



extern float Xsize(class basecanvas *canvas, std::string str);
extern float Ysize(class basecanvas *canvas, std::string str);
extern float get_scale(void);