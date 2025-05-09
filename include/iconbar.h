#pragma once

#include "gridcad.h"

#define ICON_ZERO 0
#define ICON_ONE  1
#define ICON_NAND 2

class icon {
public:
    icon(SDL_Renderer *renderer, int type);
    ~icon(void);
    
    class element *create_element(void);
    void draw (SDL_Renderer *renderer, float X1, float Y1, float width, float height);
private:
    SDL_Texture *texture;
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
