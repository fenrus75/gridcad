#include <SDL2/SDL.h>


#include "gridcad.h"


int main(int argc, char **argv)
{
    class scene *_scene;
    class element * _element;
    SDL_Init(SDL_INIT_VIDEO);
    
    _scene = new scene();
    
    _element = new element(3,5);
    _element->place(5,7);
    
    _scene->add_element(_element);
    
    _scene->eventloop();
    
    
    delete _scene;
    
    
    SDL_Quit();
    return 0;
}
