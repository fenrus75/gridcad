#pragma once

#include "wirepath.h"
#include <SDL2/SDL.h>

#include <vector>


#define COLOR_BACKGROUND_MAIN 0
#define COLOR_BACKGROUND_GRID 1
#define COLOR_ELEMENT_NORMAL 2
#define COLOR_ELEMENT_GHOST 3
#define COLOR_ELEMENT_DND 4
#define COLOR_ELEMENT_ORIGIN 5
#define COLOR_ELEMENT_INSIDE 0
#define COLOR_ELEMENT_CONNECTOR COLOR_ELEMENT_NORMAL
#define COLOR_WIRE_SOLID 6
#define COLOR_WIRE_INVALID 7
#define COLOR_WIRE_MOTION 8
#define COLOR_VALUE_RED 9
#define COLOR_VALUE_GREEN 10

class element;
class wire;
class port;
struct value;


#define VALUE_TYPE_BOOL 0
#define VALUE_TYPE_INT 1
#define VALUE_TYPE_FLOAT 2
#define VALUE_TYPE_ARRAY 3
struct value {
    int type;
    union {
        bool boolval;
        int intval;
        float floatval;
        int arrayval[64];
    };
};



class scene
{
public:
    scene(void);
    virtual ~scene(void);
    
    void draw(void);
    void eventloop(void);
    
    
    void drawBox(float X1, float Y1, float X2, float Y2, int color);
    void drawCircle(float X, float Y, float R, int color, int color2=COLOR_BACKGROUND_MAIN);
    void drawLine(float X1, float Y1, float X2, float Y2, int color);
    
    int X_to_scr(float X);
    int Y_to_scr(float Y);
    float scr_to_X(int X);
    float scr_to_Y(int Y);
    
    float offsetX, offsetY;
    float scaleX, scaleY;
    
    float sizeX, sizeY;
    
    void add_element(class element *element);
    
    bool can_place_element(float x, float y, int w, int h, class element *myself = NULL);

    class port * is_port(float X, float Y); /* X and Y are global positions */
    
    void fill_grid(class wiregrid* grid);
    
    
protected:
    std::vector<class element *> elements;
    SDL_Window *window;
    SDL_Renderer *renderer;
    class element *dragging;
    class port *dragging_port;
    class wire *dragging_wire;
    bool left_mouse_down;
    float mouseX, mouseY;
    
};


#define DRAW_NORMAL 0
#define DRAW_GHOST 1
#define DRAW_DND 2
#define DRAW_ORIGIN 3

class port {
public:
    int X, Y;
    float screenX, screenY;
    const char *name;
    class element *parent;
    struct value value;
};

class element
{
public:    
    element(int sizeX, int sizeY, const char *_name);
    virtual ~element(void);
    
    void place(int X, int Y);
    
    void drawAt(class scene *scene, float X, float Y, int type);
    virtual void draw(class scene *scene, int type);
    
    void start_drag(float X, float Y);
    void update_drag(class scene *scene, float X, float Y);
    void stop_drag(class scene *scene);
    
    bool intersect(float X, float Y);
    
    const char * get_name(void) { return name; };
    
    void add_port(int X, int Y, const char *name);
    virtual void fill_grid(class wiregrid* grid);
    
    class port * is_port(float X, float Y); /* X and Y are global positions */
    
    void add_wire(class wire *wire, class port *port);
    float get_X(void) { return X; };
    float get_Y(void) { return Y; };
    
    void update_value(class port *port, struct value *value);
protected:

    const char *name;
    int sizeX;
    int sizeY;
    bool over_drag_threshold;  /* has a drag-and-drop been far enough to avoid spurious drags */
    
    
    float X, Y;
    float Xghost, Yghost;
    float Xdnd, Ydnd;
    float X_in_drag, Y_in_drag;
    
    std::vector<class port *> ports;
    std::vector<class wire *> wires;
    
    void drawConnector(class scene *scene, float X, float Y, int cX, int cY, int type);    
};

class wire {
public:
    wire(int x1, int y1, int x2, int y2, int _color = COLOR_WIRE_SOLID);
    ~wire(void);
    
    void move_target(int x2, int y2);
    void draw(class scene *, int color = COLOR_WIRE_SOLID);
    void route(class scene *);
    
    void get_ref(void);
    void add_parent(class port *port);
    void reseat(void);
    
    void update_value(struct value *newvalue);
    
protected:
    struct value value;
    std::vector<class port *> parents;
    int X1, Y1, X2, Y2;
    int color;
    int refcount;
    std::vector<struct waypoint> *points;
};


class connector : public element
{
public:
    connector(float _X = 0, float _Y =0);
    virtual ~connector();
    virtual void draw(class scene *scene, int type);
    virtual void fill_grid(class wiregrid* grid);
};

extern int R(int color);
extern int G(int color);
extern int B(int color);
extern int Alpha(int color);
extern int value_color(struct value *value);

