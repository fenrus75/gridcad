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

#pragma once

#ifndef __INCLUDE_GUARD_
#define __INCLUDE_GUARD_

#include "wirepath.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <vector>
#include <string>
#include <map>

#include <nlohmann/json_fwd.hpp>
using json = nlohmann::json;

#define COLOR_BACKGROUND_MAIN 0
#define COLOR_BACKGROUND_GRID 1
#define COLOR_ELEMENT_NORMAL 2
#define COLOR_ELEMENT_GHOST 3
#define COLOR_ELEMENT_DND 4
#define COLOR_ELEMENT_ORIGIN 5
#define COLOR_ELEMENT_INSIDE 0
#define COLOR_ELEMENT_CONNECTOR COLOR_ELEMENT_NORMAL
#define COLOR_WIRE_SOLID 6
#define COLOR_WIRE_INVALID 7
#define COLOR_WIRE_MOTION 8
#define COLOR_VALUE_RED 9
#define COLOR_VALUE_GREEN 10
#define COLOR_ERROR_WHITE 11
#define COLOR_ERROR_BLACK 12
#define COLOR_AREA_SELECT 13
#define COLOR_BACKGROUND_YELLOW 14

class element;
class wire;
class port;
class iconbar;
class basecanvas;
struct value;



class base
{
public:
    virtual std::string class_id(void) = 0;
};



class basecanvas
{
public:
    basecanvas(void);
    virtual ~basecanvas(void);
    virtual bool handle_event(SDL_Event &event) = 0;
    virtual void draw(void) = 0;
    unsigned int get_window_ID(void) { return windowID;};


    int X_to_scr(float X);
    int Y_to_scr(float Y);
    float scr_to_X(int X);
    float scr_to_Y(int Y);
    

    void draw_box(float X1, float Y1, float X2, float Y2, int color, int alpha = -1);
    void draw_circle(float X, float Y, float R, int color, int color2=COLOR_BACKGROUND_MAIN);
    void draw_circle2(float X, float Y, float R, int color, int color2=COLOR_BACKGROUND_MAIN);
    void draw_line(float X1, float Y1, float X2, float Y2, int color);
    void draw_shadow_Line(float X1, float Y1, float X2, float Y2, int color);

    SDL_Texture *load_image(const char *filename);
    SDL_Texture *load_image(std::string filename);
    void draw_image(SDL_Texture *image, float X, float Y, float W, float H, int alpha=255, bool keepaspect = false);
    void draw_image(std::string filename, float X, float Y, float W, float H, int alpha=255, bool keepaspect = false);
    void draw_image_rotated(SDL_Texture *image, float X, float Y, float W, float H, int alpha=255, int angle = 0);
    void draw_image_rotated(std::string filename, float X, float Y, float W, float H, int alpha=255, int angle = 0);
    SDL_Texture *text_to_texture(const char *text);
    SDL_Texture *text_to_texture(std::string text);
    void draw_text(std::string text, float X, float Y, float W, float H);
    void unhide(void);

protected:
    float offsetX = 0.0;
    float offsetY = 0.0;
    float scaleX = 25.0, scaleY = 25.0;
    unsigned int windowID = 0;    
    SDL_Renderer *renderer = NULL;
    SDL_Window *window = NULL;
    TTF_Font *font = NULL;
    std::map<std::string, SDL_Texture *> texture_cache;
    std::map<std::string, SDL_Texture *> text_cache;
};

/* gui canvas to draw on */
class canvas : public basecanvas
{
public:
    canvas (class scene *_scene);
    virtual ~canvas(void);
    
    void draw(void) override;
    
    class scene *get_scene(void) { return current_scene; };
    class scene *swap_scene(class scene *scene);
    bool handle_event(SDL_Event &event) override;
    

    void to_json(json& j);
    class scene *get_undo(void);
    void take_undo_snapshot(class scene *scene);
    void from_json_to_floating(json &j);
    
    void update_window_title(void);
         
protected:
    bool draw_grid = false;
    bool window_shown = true;
    class scene *current_scene = NULL;
    SDL_Rect main_area_rect, ui_area_rect;
    class element *dragging = NULL;
    std::vector<class element *> floating;
    class port *dragging_port = NULL;
    class wire *dragging_wire = NULL, *hover_wire = NULL;
    class iconbar *icon_bar = NULL;
    class icon *active_icon = NULL;
    bool left_mouse_down = false;
    float mouseX = 0.0, mouseY = 0.0;
    float click_start_X = 0.0, click_start_Y = 0.0;
    int middle_X = 0, middle_Y = 0;
    bool shift_down = false; /* not saved in json */
    std::vector<std::string> undo_list;
    bool in_area_select = false;
    float area_select_X1 = 0.0, area_select_Y1 = 0.0;
    SDL_Texture *area_select_texture = NULL;

    bool handle_event_iconarea(SDL_Event &event);
    bool handle_event_drawingarea(SDL_Event &event);
    
};

/* logical representation of a (sub)circuit */
class scene : public base
{
public:
    scene(std::string name, std::string parent = "");
    virtual ~scene(void);
    
    virtual std::string class_id(void) { return "scene:";};

    float sizeX, sizeY;
    
    void add_element(class element *element);
    void remove_element(class element *element);
    
    bool can_place_element(float x, float y, int w, int h, class element *myself = NULL);

    class port * is_port(float X, float Y); /* X and Y are global positions */
    class wire *is_wire(float X, float Y);
    
    void fill_grid(class wiregrid* grid);
    
    void deselect_all(void);
    class element *selected_element(void);
    
    void to_json(json& j);
    void selection_to_json(json& j);
    void from_json(json& j);
    void process_delete_requests(void);
    void delete_selection(void);

    /* needs to become protected */
    std::vector<class element *> elements;
    void rewire_section(int x1, int y1, int w, int h);
    void rewire_section(class element *element);
    void remove_orphans(void);

    unsigned int selected_count(void);
    
    std::string get_name(void) { return name; };
    std::string get_full_name(void) { return parental_name + "/" + name;};
    void update_name(std::string _name) { name = _name;};
    uint64_t get_generation_count(void) { return generation_count; };
    
protected:
    std::string name = "";
    std::string parental_name = "";    
    uint64_t generation_count = 0;
};


#define DRAW_NORMAL 0
#define DRAW_GHOST 1
#define DRAW_DND 2
#define DRAW_ORIGIN 3


class element : public base
{
public:    
    element(int sizeX, int sizeY, std::string _name, std::string _parent = "");
    virtual ~element(void);

    virtual std::string class_id(void) { return "element:";};
    
    void place(int X, int Y);
    
    virtual void drawAt(class canvas *canvas, float X, float Y, int type);
    virtual void draw(class canvas *canvas, int type);
    virtual void draw_phase2(class canvas *canvas, int type);
    
    void start_drag(float X, float Y);
    void update_drag(class canvas *canvas, class scene *scene,  float X, float Y);
    void stop_drag(class canvas *canva);
    
    virtual bool intersect(float X, float Y);
    
    const std::string  get_name(void) { return name; };
    const std::string  get_full_name(void) { return parental_name + "/" + name; };
    
    void add_port(int X, int Y, const char *name, int direction = 0, int _width = 0);
    virtual void fill_grid(class wiregrid* grid);
    
    class port * is_port(float X, float Y); /* X and Y are global positions */
    
    float get_X(void) { return X; };
    float get_Y(void) { return Y; };
    float get_width(void) { return sizeX; };
    float get_height(void) { return sizeY; };
    
    void update_value(class port *port, struct value *value, int ttl);
    virtual void notify(int ttl);
    virtual void calculate(int ttl);
    virtual bool mouse_select(float X, float Y); /* scene absolute */
    bool has_moved(void) { return over_drag_threshold;};
    class wire *is_wire(float X, float Y);
    
    void reseat(void);
    virtual void to_json(json& j);
    virtual void from_json(json& j);
    void select(void) { selected = true; single = false; reseat();};
    void select_single(void) { selected = true; single = true; reseat();};
    void deselect(void) { selected = false; single = false;};
    bool is_selected(void) { return selected;};
    virtual bool want_deleted(void);
    void delete_if_selected(void);
    void remove_orphans(void);
    
    virtual bool in_edit_mode(void) { return false; };
    
    virtual void handle_event(class canvas *canvas, SDL_Event &event);
    
    std::string get_uuid(void) { return uuid;};
    void reset_uuid(void);
    void delete_element(void);
    
    virtual void rotate_ports(void) {};
    void update_name(std::string _name) { name = _name;};
    void update_parental_name(std::string _name) { parental_name = _name;};
    void hover_ports(canvas *canvas);


    
protected:
    std::string uuid = "";
    std::string name = "";
    std::string parental_name = "";    
    int sizeX = 1;
    int sizeY = 1;
    int angle = 0;
    bool over_drag_threshold = false;  /* has a drag-and-drop been far enough to avoid spurious drags */
    bool want_delete = false; /* not saved to disk */
    
    
    float X = 0, Y = 0;
    float Xghost = 0, Yghost = 0;
    float Xdnd = 0, Ydnd = 0;
    float X_in_drag = 0, Y_in_drag = 0;
    bool selected = false; /* not saved to disk */
    bool single = false; /* only selected item in the scene - not saved to disk */
    float mouseX = -100, mouseY = -100; /* nosave */
    
    std::vector<class port *> ports;
    
};



extern int R(int color);
extern int G(int color);
extern int B(int color);
extern int Alpha(int color);
extern int value_color(struct value *value);
extern class element *element_from_class_id(std::string classid);

extern bool wire_factory_force_new_name;

extern class wire *json_wire_factory(json &jwire);
extern void clear_wire_factory(void);

extern void register_new_canvas(class basecanvas *canvas);
extern void unregister_canvas(class basecanvas *canvas);
extern void labelevent(SDL_Event &event, std::string *text);

extern std::string generate_semi_uuid(void);
extern void populate_library(std::string directory);
#endif