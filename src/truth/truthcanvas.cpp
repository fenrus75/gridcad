/*
 * gridcad -- a tool to do digital circuit design with a GUI
 *
 * (C) Copyright 2025  -- Arjan van de Ven
 *
 * Licensed under the terms of the
 * GNU General Public LIcense version 3.0
 *
 * Authors:
 *    Arjan van de Ven <arjanvandeven@gmail.com>
 */

#include "gridcad.h"
#include "truthcanvas.h"
#include "model_truth.h"
#include "widget.h"
#include <cstdlib>


truthcanvas::truthcanvas(class model_truth *_element)
{

    element = _element;

    window = SDL_CreateWindow(("GridCad Truth Table / "+ _element->get_full_name()).c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_RESIZABLE);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    windowID = SDL_GetWindowID(window);

    fill_grid();
    unhide();
    draw();    
}


truthcanvas::~truthcanvas(void)
{
	unregister_canvas(this);
	SDL_DestroyRenderer(renderer);
	renderer = NULL;
	SDL_DestroyWindow(window);
}

void truthcanvas::clear_widgets(void)
{
    unsigned int i;
    
    for (i = 0; i < widgets.size(); i++) {
        auto widget = widgets[i];
        delete widget;
        widgets[i] = nullptr;
    }
    widgets.clear();
}


static float calcX(int x, int inputs)
{
    if (x < inputs)
        return (1.1 + x * 5);
    else
        return (1.9 + x * 5);
}

static int hdist_strict(std::vector<char> A, std::vector<char> B)
{
	unsigned int x;
	int dist = 0;
	
	for (x = 0; x < A.size(); x++) {
		if (A[x] !=  B[x])
			dist++;
	}
	
	return dist;
}

static int hdist_first(std::vector<char> A, std::vector<char> B)
{
	unsigned int x;
	
	for (x = 0; x < A.size(); x++) {
		if (A[x] != B[x])
			return x;
	}
	return -1;
}

void truthcanvas::clear_canX(void)
{
     unsigned int y;
     unsigned int x;
     for (y = 0; y < canX.size(); y++)     
         for (x = 0; x < canX[y].size(); x++)
            canX[y][x] = false;
}

void truthcanvas::do_canX(unsigned int Y)
{
     unsigned int y;

     for (y = 0; y < canX.size(); y++) {
         if (y == Y)
            continue;
         if (hdist_strict(element->values[y], element->values[Y]) == 1) {
            int off = hdist_first(element->values[y], element->values[Y]);
            if (off >=0) {
                canX[Y][off] = true;
                canX[y][off] = true;
            }
         }   
     }
}

void truthcanvas::fill_grid(void)
{
	unsigned int x,y;
	unsigned int inputs = element->get_inputs();
	
	clear_widgets();
	canX.resize(element->values.size());
	for (y = 0; y < canX.size(); y++) {
	  canX[y].resize(element->values[0].size());
	  for (x = 0; x < canX[y].size(); x++)
	     canX[y][x] = false;
        }
	  
	  

        add_widget(new class slider(2,1,11,1, "Reverse input port order", &element->flip_input_ports));         
        add_widget(new class slider(15,1,11,1, "Reverse output port order", &element->flip_output_ports));         
	
	add_widget(new class button(calcX(inputs-1, inputs), 2, 2.4,1.8, "assets/model_truth/minus.png", ACTION_DEL_INPUT, this));
	add_widget(new class button(calcX(inputs-1, inputs)+2.5, 2, 2.4,1.8, "assets/model_truth/plus.png", ACTION_ADD_INPUT, this));

	add_widget(new class button(calcX(element->values[0].size()-1, inputs), 2, 2.4,1.8, "assets/model_truth/minus.png", ACTION_DEL_OUTPUT, this));
	add_widget(new class button(calcX(element->values[0].size()-1, inputs)+2.5, 2, 2.4,1.8, "assets/model_truth/plus.png", ACTION_ADD_OUTPUT, this));

	for (x = 0; x < element->names.size(); x++) 
		add_widget(new class label(calcX(x, inputs), 3+1.1, 4.8, 1.8, &element->names[x]));    


	for (y = 0; y < element->values.size(); y++) {
		for (x = 0; x < element->values[y].size(); x++) {
			add_widget(new class tristate(calcX(x, inputs), 3+1.1 + (y + 1) * 2, 4.8, 1.8, &element->values[y][x], x, y, x < inputs, this));    
		}
	}
}

void truthcanvas::draw(void)
{
    SDL_SetRenderDrawColor(renderer, R(COLOR_BACKGROUND_GRID),
			       G(COLOR_BACKGROUND_GRID),
			       B(COLOR_BACKGROUND_GRID),
			       Alpha(COLOR_BACKGROUND_GRID));
			       
    if (need_fill_grid) {
        need_fill_grid = false;
        fill_grid();
    }
        
    unsigned int row = rand() % canX.size();
    do_canX(row);
			       
    SDL_RenderClear(renderer);
    
    for (auto widget : widgets)
      widget->draw(this);
      
    SDL_RenderPresent(renderer);
}

bool truthcanvas::handle_event(SDL_Event &event)
{
    for (auto widget : widgets)
        widget->handle_event(event);
        
    if (event.type == SDL_KEYDOWN) {
         element->names_to_ports();
    }
        
    switch (event.type) {
    case SDL_KEYDOWN:
         switch (event.key.keysym.sym) {
	 case SDLK_UP:
	       for (auto widget : widgets)
	          if (widget->get_gridX() == selectX && widget->get_gridY() == selectY-1 && selectY >= 1) {
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
         element->names_to_ports();
               
          break;
   
    case SDL_WINDOWEVENT:
         if (event.window.event == SDL_WINDOWEVENT_CLOSE) {  /* user clicked the 'x' */
              SDL_HideWindow(window); /* TODO: THis leaks like a sieve */
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

void truthcanvas::unhide(void)
{
    SDL_ShowWindow(window);
}

void truthcanvas::add_output(void)
{
     element->add_output();
     need_fill_grid = true;
     element->calculate(20);
}
void truthcanvas::del_output(void)
{
     element->del_output();
     need_fill_grid = true;  /* we can't actually redo the grid since this is called from places inside iterators over the grid */
     element->calculate(20);
}
void truthcanvas::add_input(void)
{
     element->add_input();
     need_fill_grid = true;
     element->calculate(20);
}
void truthcanvas::del_input(void)
{
     element->del_input();
     need_fill_grid = true;
     element->calculate(20);
}

void truthcanvas::turn_from_X(unsigned int X, unsigned int Y)
{
     element->turn_from_X(X,Y);
     need_fill_grid = true;
     element->calculate(20);
}

void truthcanvas::turn_to_X(unsigned int X, unsigned int Y)
{
     element->turn_to_X(X,Y);
     need_fill_grid = true;
     element->calculate(20);
}

void truthcanvas::values_changed(void)
{
     element->calculate(20);
}