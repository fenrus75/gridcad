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

class element;
class wire;
struct port;

class scene
{
public:
    scene(void);
    ~scene(void);
    
    void draw(void);
    void eventloop(void);
    
    
    void drawBox(float X1, float Y1, float X2, float Y2, int color);
    void drawCircle(float X, float Y, float R, int color);
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

    struct port * is_port(float X, float Y); /* X and Y are global positions */
    
    void fill_grid(class wiregrid* grid);
    
    
private:
    std::vector<class element *> elements;
    SDL_Window *window;
    SDL_Renderer *renderer;
    class element *dragging;
    struct port *dragging_port;
    class wire *dragging_wire;
    bool left_mouse_down;
    float mouseX, mouseY;
    
};


#define DRAW_NORMAL 0
#define DRAW_GHOST 1
#define DRAW_DND 2
#define DRAW_ORIGIN 3

struct port {
    int X, Y;
    float screenX, screenY;
    const char *name;
    class element *parent;
};

class element
{
public:    
    element(int sizeX, int sizeY, const char *_name);
    ~element(void);
    
    void place(int X, int Y);
    
    void drawAt(class scene *scene, float X, float Y, int type);
    void draw(class scene *scene, int type);
    
    void start_drag(float X, float Y);
    void update_drag(class scene *scene, float X, float Y);
    void stop_drag(class scene *scene);
    
    bool intersect(float X, float Y);
    
    const char * get_name(void) { return name; };
    
    void add_port(int X, int Y, const char *name);
    void fill_grid(class wiregrid* grid);
    
    struct port * is_port(float X, float Y); /* X and Y are global positions */
    
    void add_wire(class wire *wire, struct port *port);
    float get_X(void) { return X; };
    float get_Y(void) { return Y; };
private:

    const char *name;
    int sizeX;
    int sizeY;
    bool over_drag_threshold;  /* has a drag-and-drop been far enough to avoid spurious drags */
    
    
    float X, Y;
    float Xghost, Yghost;
    float Xdnd, Ydnd;
    float X_in_drag, Y_in_drag;
    
    std::vector<struct port *> ports;
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
    void add_parent(struct port *port);
    void reseat(void);
    
private:
    std::vector<struct port *> parents;
    int X1, Y1, X2, Y2;
    int color;
    int refcount;
    std::vector<struct waypoint> *points;
};



int R(int color);
int G(int color);
int B(int color);
int Alpha(int color);
