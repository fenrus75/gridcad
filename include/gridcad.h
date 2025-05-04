#include <SDL2/SDL.h>

#include <vector>

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
    
    
private:
    SDL_Window * window;
    SDL_Renderer * renderer;
};


class element
{
};

#define COLOR_BACKGROUND_MAIN 0
#define COLOR_BACKGROUND_GRID 1


int R(int color);
int G(int color);
int B(int color);
int Alpha(int color);