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
class iconbar;
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


/* gui canvas to draw on */
class canvas
{
public:
    canvas (class scene *_scene);
    virtual ~canvas(void);
    
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
    
    
    SDL_Texture *load_image(const char *filename);
    void draw_image(SDL_Texture *image, float X, float Y, float W, float H, int alpha=255);
    SDL_Texture *text_to_texture(const char *text);
    
    class scene *get_scene(void) { return current_scene; };
    
    
protected:
    class scene *current_scene;
    SDL_Renderer *renderer = NULL;
    SDL_Window *window = NULL;
    SDL_Rect main_area_rect, ui_area_rect;
    class element *dragging = NULL, *floating = NULL;
    class port *dragging_port = NULL;
    class wire *dragging_wire = NULL, *hover_wire = NULL;
    class iconbar *icon_bar = NULL;
    class icon *active_icon = NULL;
    bool left_mouse_down = false;
    float mouseX, mouseY;
    float click_start_X, click_start_Y;
    
};

/* logical representation of a (sub)circuit */
class scene
{
public:
    scene(void);
    virtual ~scene(void);

    float sizeX, sizeY;
    
    void add_element(class element *element);
    
    bool can_place_element(float x, float y, int w, int h, class element *myself = NULL);

    class port * is_port(float X, float Y); /* X and Y are global positions */
    class wire *is_wire(float X, float Y);
    
    void fill_grid(class wiregrid* grid);
    
    std::vector<class element *> elements;
    
protected:
    
};


#define DRAW_NORMAL 0
#define DRAW_GHOST 1
#define DRAW_DND 2
#define DRAW_ORIGIN 3

#define PORT_IN 0
#define PORT_OUT 1
#define PORT_INOUT 2

class port {
public:
    port(const char *_name, int _direction = PORT_IN);
    ~port(void);
    int X, Y;
    float screenX, screenY;
    const char *name = NULL;
    class element *parent = NULL;
    struct value value = {};
    void add_wire(class wire *wire);
    void update_value(struct value *newvalue);
    int direction = PORT_IN;;
    
    void draw(class canvas *canvas, int color);
    void draw_wires(class canvas *canva);
    void drawAt(class canvas *canvas, float X, float Y, int color);
    void stop_drag(class canvas *canva);
    virtual void notify(void);
    class wire *is_wire(float X, float Y);
    
    void remove_wire(class wire *wire);
private:
    void drawConnector(class canvas *canvas, float X, float Y, int cX, int cY, int type);    
    std::vector<class wire*> wires;
    SDL_Texture *label = NULL;
};

class element
{
public:    
    element(int sizeX, int sizeY, const char *_name);
    virtual ~element(void);
    
    void place(int X, int Y);
    
    virtual void drawAt(class canvas *canvas, float X, float Y, int type);
    virtual void draw(class canvas *canvas, int type);
    
    void start_drag(float X, float Y);
    void update_drag(class canvas *canvas, class scene *scene,  float X, float Y);
    void stop_drag(class canvas *canva);
    
    virtual bool intersect(float X, float Y);
    
    const char * get_name(void) { return name; };
    
    void add_port(int X, int Y, const char *name, int direction = 0, bool initval = false);
    virtual void fill_grid(class wiregrid* grid);
    
    class port * is_port(float X, float Y); /* X and Y are global positions */
    
    float get_X(void) { return X; };
    float get_Y(void) { return Y; };
    
    void update_value(class port *port, struct value *value);
    virtual void notify(void);
    virtual void calculate();
    virtual void mouse_select(void);
    bool has_moved(void) { return over_drag_threshold;};
    class wire *is_wire(float X, float Y);
protected:

    const char *name = NULL;
    int sizeX = 1;
    int sizeY = 1;
    bool over_drag_threshold = false;  /* has a drag-and-drop been far enough to avoid spurious drags */
    
    
    float X = 0, Y = 0;
    float Xghost = 0, Yghost = 0;
    float Xdnd = 0, Ydnd = 0;
    float X_in_drag = 0, Y_in_drag = 0;
    
    std::vector<class port *> ports;
    
};

class wire {
public:
    wire(int x1, int y1, int x2, int y2, int _color = COLOR_WIRE_SOLID);
    virtual ~wire(void);
    
    void move_target(int x2, int y2);
    void draw(class canvas *, int color = COLOR_WIRE_SOLID);
    void route(class scene *);
    
    void get_ref(void);
    void add_port(class port *port);
    void reseat(void);
    
    void update_value(struct value *newvalue);
    virtual void notify(void);
    struct value value;
    bool intersect(float targetX, float targetY);
    
    void select(void) { selected = true;};
    void deselect(void) { selected = false;};
    void remove(void);
    
protected:
    bool selected = false;
    std::vector<class port *> ports;
    int X1 = 0, Y1 = 0, X2 = 0, Y2 = 0;
    int color = 0;
    int refcount = 0;
    std::vector<struct waypoint> *points = NULL;
};


class connector : public element
{
public:
    connector(float _X = 0, float _Y =0);
    virtual ~connector();
    void draw(class canvas *canvas, int type) override;
    void fill_grid(class wiregrid* grid) override;
};

extern int R(int color);
extern int G(int color);
extern int B(int color);
extern int Alpha(int color);
extern int value_color(struct value *value);

