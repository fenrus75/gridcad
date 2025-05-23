#include "gridcad.h"
#include "truthcanvas.h"
#include "model_truth.h"

truthcanvas::truthcanvas(class model_truth *_element)
{
    unsigned int x,y;

    element = _element;
    unsigned int inputs = element->invalues[0].size();

    window = SDL_CreateWindow("GridCad Truth Table", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1024, 768, SDL_WINDOW_RESIZABLE);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    windowID = SDL_GetWindowID(window);

    for (x = 0; x < element->invalues[0].size(); x++) 
        add_widget(new class label(1.1 + x *5, 1.1, 4.8, 1.8, _element->invalues[0][x].name));    
    for (x = 0; x < element->outvalues[0].size(); x++) 
        add_widget(new class label(1.4 + (x+inputs) *5, 1.1, 4.8, 1.8, _element->outvalues[0][x].name));    


    for (y = 1; y < element->invalues.size(); y++) {
         for (x = 0; x < element->invalues[y].size(); x++) {
             add_widget(new class tristate(1.1 + x * 5, 1.1 + y * 2, 4.8, 1.8, &_element->invalues[y][x]));    
         }
    }

    for (y = 1; y < element->outvalues.size(); y++) {
         for (x = 0; x < element->outvalues[y].size(); x++) {
             add_widget(new class tristate(1.4 + (x+inputs) * 5, 1.1 + y * 2, 4.8, 1.8, &_element->outvalues[y][x]));    
         }
    }
    
}


truthcanvas::~truthcanvas(void)
{
}

void truthcanvas::draw(void)
{
    SDL_SetRenderDrawColor(renderer, R(COLOR_BACKGROUND_GRID),
			       G(COLOR_BACKGROUND_GRID),
			       B(COLOR_BACKGROUND_GRID),
			       Alpha(COLOR_BACKGROUND_GRID));
			       
    SDL_RenderClear(renderer);
    
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

void truthcanvas::add_widget(class widget *widget)
{
     widgets.push_back(widget);
}