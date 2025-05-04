#include <SDL2/SDL.h>

#include "gridcad.h"


scene::scene(void)
{
    window = SDL_CreateWindow("FOO BAR", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1024, 768, 0);
    renderer = SDL_CreateRenderer(window, -1, 0);
    
    offsetX = 0;
    offsetY = 0;
    scaleX = 25;
    scaleY = 25;
    
    sizeX = 200;
    sizeY = 200;
}

scene::~scene(void)
{
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}


void scene::eventloop(void)
{
    SDL_Event event;
    int ret = 0;
    bool leave = false;
    
    
    while (!leave) {
        if (!ret)
            SDL_Delay(1);
        ret = SDL_PollEvent(&event);
        
        switch (event.type) {
            case SDL_QUIT:
                leave = true;
                break;
        }
        
        draw();
    }
}

void scene::draw(void)
{
        SDL_RenderPresent(renderer);     
}