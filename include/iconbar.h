#pragma once

#include "gridcad.h"


class icon {
public:
    icon(void);
    ~icon(void);
    
    class element *create_element(void);
    void draw (SDL_Renderer *renderer, int X1, int Y1);
private:
};

class iconbar {
public:
    iconbar(SDL_Renderer *_renderer, SDL_Rect _rect);
    ~iconbar(void);
    
    void resize(SDL_Rect _rect);
    
    void draw(void);
private:
    SDL_Rect rect;
    std::vector<std::vector<class icon *>> icons;
    SDL_Renderer *renderer;
};
