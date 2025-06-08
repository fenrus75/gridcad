#pragma once

#include "gridcad.h"

#include "library.h"
#include "contextmenu.h"

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
#define ICON_1TO4 14
#define ICON_8TO4 15
#define ICON_4TO8 16
#define ICON_DATASCOPE 17
#define ICON_DFLIPFLOP 18
#define ICON_DELAYLINE 19
#define ICON_CLOCK 20

class icon {
public:
    icon(SDL_Renderer *renderer, std::string class_id, std::string filename, std::string tooltip);
    virtual ~icon(void);
    
    virtual class element *create_element(void);
    void draw (SDL_Renderer *renderer, float X1, float Y1, float width, float height);
    void set_active(void) { active = true; };
    void set_inactive(void) { active = false; };
    void assign_library_element(struct library_block block);
    std::string get_tooltip(void) { return tooltip; };
private:
    bool active  = false;
    std::string class_id = "";
    std::string tooltip = "";
    struct library_block lib = {}; 
    SDL_Renderer *_renderer = NULL;
    SDL_Texture *texture = NULL;
    class element *library_element(void);
};

class oneiconbar {
public:
    oneiconbar(SDL_Renderer *_renderer, SDL_Rect _rect, std::string name);
    virtual ~oneiconbar(void);
    
    void resize(SDL_Rect _rect);
    
    void draw(void);
    void handle_event(SDL_Event &event);    
    class icon *current_icon(int ScreenX, int ScreenY);
    std::string current_tooltip(int ScreenX, int ScreenY);
    void set_element(unsigned int X, unsigned int Y, std::string class_id, std::string filename, std::string tooltip, struct library_block *block = NULL);
    void set_element(std::string class_id, struct library_block *block = NULL);
    void set_element(std::string class_id, std::string filename, std::string tooltip, struct library_block *block = NULL);
    void insert_blank(void);
    std::string get_name(void) { return name; };

private:    
    unsigned int cIndex = 0; 
    SDL_Rect rect = {};
    std::vector<std::vector<class icon *>> icons;
    SDL_Renderer *renderer = NULL;
    std::string name;

};

class iconbar {
public:
    iconbar(SDL_Renderer *_renderer, SDL_Rect _rect);
    virtual ~iconbar(void);
    
    void resize(SDL_Rect _rect);
    
    void draw(void);
    void handle_event(SDL_Event &event);    
    void next(void);
    void previous(void);
    void set(class oneiconbar *b);
    class icon *current_icon(int ScreenX, int ScreenY);
    std::string current_tooltip(int ScreenX, int ScreenY);
    class contextmenu *get_menu(int ScreenX, int ScreenY);
    
private:
    SDL_Rect rect = {};
    unsigned int active_index = 0;
    std::vector<class oneiconbar *> icons;
    class oneiconbar *current_icons = NULL;
    SDL_Renderer *renderer = NULL;
    std::map<std::string, class oneiconbar*> libtabs;
    class icon_contextmenu *menu = NULL;
    SDL_Texture *dropdown = NULL;
};


