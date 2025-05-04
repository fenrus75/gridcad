#include <SDL2/SDL.h>

#include <vector>

class scene
{
public:
    scene(void);
    ~scene(void);
    
    void draw(void);
    void eventloop(void);
    
    
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
