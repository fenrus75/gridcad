#include <SDL2/SDL.h>


#include "gridcad.h"


int main(int argc, char **argv)
{
    class scene *_scene;
    SDL_Init(SDL_INIT_VIDEO);
    
    _scene = new scene();
    
    _scene->eventloop();
    
    
    delete _scene;
    
    
    SDL_Quit();
    return 0;
}
