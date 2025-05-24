#include "gridcad.h"
#include "truthcanvas.h"
#include "model_truth.h"

truthcanvas::truthcanvas(class model_truth *_element)
{
    unsigned int x,y;

    element = _element;
    unsigned int inputs = element->get_inputs();

    window = SDL_CreateWindow("GridCad Truth Table", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_RESIZABLE);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    windowID = SDL_GetWindowID(window);

    for (x = 0; x < element->values[0].size(); x++) 
        add_widget(new class label(1.1 + x*5, 1.1, 4.8, 1.8, _element->names[x]));    


    for (y = 0; y < element->values.size(); y++) {
         for (x = 0; x < element->values[y].size(); x++) {
             add_widget(new class tristate(1.1 + x * 5, 1.1 + (y + 1) * 2, 4.8, 1.8, &_element->values[y][x], x, y, x < inputs));    
         }
    }
    draw();    
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
        
    switch (event.type) {
    case SDL_KEYDOWN:
         switch (event.key.keysym.sym) {
	 case SDLK_UP:
	       for (auto widget : widgets)
	          if (widget->get_gridX() == selectX && widget->get_gridY() == selectY-1) {
                     deselect_all();
                     selectX = widget->get_gridX();
                     selectY = widget->get_gridY();
                     widget->select();
                     break;
	          }
               break;
	 case SDLK_DOWN:
	       for (auto widget : widgets)
	          if (widget->get_gridX() == selectX && widget->get_gridY() == selectY+1) {
                     deselect_all();
                     selectX = widget->get_gridX();
                     selectY = widget->get_gridY();
                     widget->select();
                     break;
	          }
               break;
	 case SDLK_LEFT:
	       for (auto widget : widgets)
	          if (widget->get_gridX() == selectX-1 && widget->get_gridY() == selectY) {
                     deselect_all();
                     selectX = widget->get_gridX();
                     selectY = widget->get_gridY();
                     widget->select();
                     break;
	          }
               break;
	 case SDLK_RIGHT:
	       for (auto widget : widgets)
	          if (widget->get_gridX() == selectX+1 && widget->get_gridY() == selectY) {
                     deselect_all();
                     selectX = widget->get_gridX();
                     selectY = widget->get_gridY();
                     widget->select();
                     break;
	          }
               break;
         }
         break;
    case SDL_MOUSEBUTTONDOWN:
          float x, y;
	  x = scr_to_X(event.motion.x);
	  y = scr_to_Y(event.motion.y);
          for (auto widget:widgets)
             if (widget->intersect(x, y)) {
               deselect_all();
               selectX = widget->get_gridX();
               selectY = widget->get_gridY();
               widget->select();
              }
               
          break;
   
    };
    
    return false;
}

void truthcanvas::add_widget(class widget *widget)
{
     widgets.push_back(widget);
}

void truthcanvas::deselect_all(void)
{
    for (auto widget : widgets)
       widget->deselect();
}