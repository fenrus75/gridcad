#include "gridcad.h"
#include "truthcanvas.h"
#include "model_truth.h"

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
    unsigned int x,y;
    unsigned int inputs = element->invalues[0].size();
    SDL_SetRenderDrawColor(renderer, R(COLOR_BACKGROUND_GRID),
			       G(COLOR_BACKGROUND_GRID),
			       B(COLOR_BACKGROUND_GRID),
			       Alpha(COLOR_BACKGROUND_GRID));
			       
    SDL_RenderClear(renderer);
    
    for (y = 0; y < element->invalues.size(); y++) {
         for (x = 0; x < element->invalues[y].size(); x++) {
                 drawBox(1.1 + x*5, 1.1 + y * 2, 5.8 + x * 5, 2.8 + y * 2, COLOR_BACKGROUND_MAIN);
                 if (y == 0)
                     draw_text(element->invalues[y][x].name,     1 + x * 5, 1 + y * 2, 4, 1.8);
                 else
                     draw_text(""+element->invalues[y][x].Token, 1 + x * 5, 1 + y * 2, 4, 1.8);
                    
         }
    }
    for (y = 0; y < element->outvalues.size(); y++) {
         for (x = 0; x < element->outvalues[y].size(); x++) {
                 drawBox(1.4 + (x + inputs) *5, 1.1 + y * 2, 6.1 + (x+inputs) * 5, 2.8 + y * 2, COLOR_BACKGROUND_MAIN);
                 if (y == 0)
                     draw_text(element->outvalues[y][x].name,     1 + (x+inputs) * 5, 1 + y * 2, 4, 1.8);
                 else
                     draw_text(""+element->outvalues[y][x].Token, 1 + (x+inputs) * 5, 1 + y * 2, 4, 1.8);
                    
         }
     }
    for (auto widget : widgets)
      widget->draw(this);
      
    SDL_RenderPresent(renderer);
}

bool truthcanvas::handleEvent(SDL_Event &event)
{
    for (auto widget : widgets)
        widget->handle_event(event);
    return false;
}