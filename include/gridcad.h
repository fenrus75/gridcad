#include <SDL2/SDL.h>

#include <vector>

class scene
{
public:
    scene(void);
    ~scene(void);
    
    void draw(void);
    void eventloop(void);
    
    
    void drawBox(float X1, float Y1, float X2, float Y2);
    
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
