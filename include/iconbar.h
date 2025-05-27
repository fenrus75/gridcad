#pragma once

#include "gridcad.h"

#include "library.h"

#define ICON_LIBRARY 0
#define ICON_ZERO 1
#define ICON_ONE  2
#define ICON_NAND 3
#define ICON_AND 4
#define ICON_NOT 5
#define ICON_TOGGLE 6
#define ICON_NOR 7
#define ICON_OR 8
#define ICON_OUTPUT 9
#define ICON_XOR 10
#define ICON_TRUTH 11
#define ICON_NEST 12
#define ICON_4TO1 13

class icon {
public:
    icon(SDL_Renderer *renderer, int type);
    virtual ~icon(void);
    
    virtual class element *create_element(void);
    void draw (SDL_Renderer *renderer, float X1, float Y1, float width, float height);
    void set_active(void) { active = true; };
    void set_inactive(void) { active = false; };
    void assign_library_element(struct library_block block);
private:
    bool active;
    int type;
    struct library_block lib;
    SDL_Renderer *_renderer;
    SDL_Texture *texture;
    class element *library_element(void);
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
    SDL_Renderer *renderer;
};


