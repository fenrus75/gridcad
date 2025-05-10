#include <SDL2/SDL.h>


#include "gridcad.h"
#include "models.h"

int main(int argc, char **argv)
{
    class scene *_scene;
    SDL_Init(SDL_INIT_VIDEO);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");
    
    _scene = new scene();
    
    _scene->eventloop();
    
    
    delete _scene;
    
    
    SDL_Quit();
    return 0;
}
