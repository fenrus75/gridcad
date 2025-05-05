#include <SDL2/SDL.h>

#include <vector>

class element;

class scene
{
public:
    scene(void);
    ~scene(void);
    
    void draw(void);
    void eventloop(void);
    
    
    void drawBox(float X1, float Y1, float X2, float Y2, int color);
    void drawCircle(float X, float Y, float R, int color);
    
    int X_to_scr(float X);
    int Y_to_scr(float Y);
    float scr_to_X(int X);
    float scr_to_Y(int Y);
    
    
    float offsetX, offsetY;
    float scaleX, scaleY;
    
    float sizeX, sizeY;
    
    void add_element(class element *element);
    
    bool can_place_element(float x, float y, int w, int h, class element *myself = NULL);
    
    
private:
    std::vector<class element *> elements;
    SDL_Window *window;
    SDL_Renderer *renderer;
    class element *dragging;
    bool left_mouse_down;
    SDL_Texture *connector_img;
    
};


#define DRAW_NORMAL 0
#define DRAW_GHOST 1
#define DRAW_DND 2
#define DRAW_ORIGIN 3

struct port {
    int X, Y;
    const char *name;
    class element *from;
    class element *to;
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
    void stop_drag(void);
    
    bool intersect(float X, float Y);
    
    const char * get_name(void) { return name; };
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
    
    void drawConnector(class scene *scene, float X, float Y, int cX, int cY, int type);    
};

#define COLOR_BACKGROUND_MAIN 0
#define COLOR_BACKGROUND_GRID 1
#define COLOR_ELEMENT_NORMAL 2
#define COLOR_ELEMENT_GHOST 3
#define COLOR_ELEMENT_DND 4
#define COLOR_ELEMENT_ORIGIN 5
#define COLOR_ELEMENT_INSIDE 0
#define COLOR_ELEMENT_CONNECTOR COLOR_ELEMENT_NORMAL


int R(int color);
int G(int color);
int B(int color);
int Alpha(int color);