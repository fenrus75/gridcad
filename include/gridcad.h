#include <SDL2/SDL.h>

#include <vector>

class scene
{
public:
    scene(void);
    
    ~scene(void);
    
    void eventloop(void);
    
    
private:
    SDL_Window * window;
    SDL_Renderer * renderer;
};


class element
{
};
