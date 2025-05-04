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
    
    int X_to_scr(float X);
    int Y_to_scr(float Y);
    
    
    float offsetX, offsetY;
    float scaleX, scaleY;
    
    float sizeX, sizeY;
    
    void add_element(class element *element);
    
    
private:
    std::vector<class element *> elements;
    SDL_Window * window;
    SDL_Renderer * renderer;
};


#define DRAW_NORMAL 0
#define DRAW_GHOST 1
#define DRAW_DND 2

class element
{
    
    element(int sizeX, int sizeY);
    
    void place(int X, int Y);
    
    void drawAt(class scene *scene, float X, float Y);
    void draw(class scene *scene, int type);
private:
    int sizeX;
    int sizeY;
    float X, Y;
    float Xghost, Yghost;
    float Xdnd, Ydnd;
    
    
};

#define COLOR_BACKGROUND_MAIN 0
#define COLOR_BACKGROUND_GRID 1
#define COLOR_ELEMENT_NORMAL 2
#define COLOR_ELEMENT_GHOST 3


int R(int color);
int G(int color);
int B(int color);
int Alpha(int color);