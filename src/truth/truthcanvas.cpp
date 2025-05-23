#include "gridcad.h"
#include "truthcanvas.h"


truthcanvas::truthcanvas(class model_truth *_element)
{
    element = _element;
    window = SDL_CreateWindow("GridCad Truth Table", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1024, 768, SDL_WINDOW_RESIZABLE);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    windowID = SDL_GetWindowID(window);
}


truthcanvas::~truthcanvas(void)
{
}

void truthcanvas::draw(void)
{
            SDL_RenderPresent(renderer);
}

bool truthcanvas::handleEvent(SDL_Event &event)
{
    return false;
}