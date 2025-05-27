#pragma once

#include "gridcad.h"

#define ICON_ZERO 0
#define ICON_ONE  1
#define ICON_NAND 2
#define ICON_AND 3
#define ICON_NOT 4
#define ICON_TOGGLE 5
#define ICON_NOR 6
#define ICON_OR 7
#define ICON_OUTPUT 8
#define ICON_XOR 9
#define ICON_TRUTH 10
#define ICON_NEST 11

class icon {
public:
    icon(SDL_Renderer *renderer, int type);
    virtual ~icon(void);
    
    virtual class element *create_element(void);
    void draw (SDL_Renderer *renderer, float X1, float Y1, float width, float height);
    void set_active(void) { active = true; };
    void set_inactive(void) { active = false; };
private:
    bool active;
    int type;
    SDL_Texture *texture;
};

class iconbar {
public:
    iconbar(SDL_Renderer *_renderer, SDL_Rect _rect);
    virtual ~iconbar(void);
    
    void resize(SDL_Rect _rect);
    
    void draw(void);
    void handle_event(SDL_Event &event);    
    class icon *current_icon(int ScreenX, int ScreenY);
private:
    SDL_Rect rect;
    std::vector<std::vector<class icon *>> icons;
    std::vector<std::vector<class icon *>> library;
    SDL_Renderer *renderer;
};

