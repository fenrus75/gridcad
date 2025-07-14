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
#include "capabilities.h"

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
#define COLOR_WIRE0	15   

class element;
class wire;
class port;
class iconbar;
class basecanvas;
struct value;
class contextmenu;
class buttonbar;
class dialog;
class scene;

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

    float scale_to_X(int X) { return X / scaleX;};
    float scale_to_Y(int Y) { return Y / scaleY;};
    

    void draw_box(float X1, float Y1, float X2, float Y2, int color, int alpha = -1);
    void draw_circle(float X, float Y, float R, int color, int color2=COLOR_BACKGROUND_MAIN);
    void draw_circle2(float X, float Y, float R, int color, int color2=COLOR_BACKGROUND_MAIN);
    void draw_line(float X1, float Y1, float X2, float Y2, int color);
    void draw_thick_line(float X1, float Y1, float X2, float Y2, int color);
    void draw_shadow_Line(float X1, float Y1, float X2, float Y2, int color);

    SDL_Texture *load_image(const char *filename);
    SDL_Texture *load_image(std::string filename);
    void draw_image(SDL_Texture *image, float X, float Y, float W, float H, int alpha=255, bool keepaspect = false);
    void draw_image(std::string filename, float X, float Y, float W, float H, int alpha=255, bool keepaspect = false);
    void draw_image_rotated(SDL_Texture *image, float X, float Y, float W, float H, int alpha=255, int angle = 0);
    void draw_image_rotated(std::string filename, float X, float Y, float W, float H, int alpha=255, int angle = 0);
    void draw_image_fragment(SDL_Texture *image, float X, float Y, float W, float H, int fromX, int fromY, int w, int h, int angle = 0);
    void draw_image_fragment(std::string filename, float X, float Y, float W, float H, int fromX, int fromY, int w, int h, int angle = 0);
    SDL_Texture *text_to_texture(const char *text);
    SDL_Texture *text_to_texture(std::string text);
    void draw_text(std::string text, float X, float Y, float W, float H);
    void draw_text_left(std::string text, float X, float Y, float W, float H);
    void unhide(void);
    bool canvas_has_focus(void) { return has_focus; };
    void set_fps(float _fps) { fps = _fps;};
    virtual float distance_from_mouse(float X, float Y) { return 0; };
    virtual class scene* get_scene(void) { return NULL; };
    virtual float screen_width(void) { return 0; };

protected:
    float offsetX = 0.0;
    float offsetY = 0.0;
    float scaleX = 25.0, scaleY = 25.0;
    
    float from_offsetX = 0.0, from_offsetY = 0.0, from_scaleX = 25.0, from_scaleY = 25.0; /* nosave */
    float target_offsetX = 0.0, target_offsetY = 0.0, target_scaleX = 25.0, target_scaleY = 25.0; /* nosave */
    uint64_t animation_start = 0; /* nosave */
    bool in_animation = false; /* nosave */
    unsigned int windowID = 0;    
    SDL_Renderer *renderer = NULL;
    SDL_Window *window = NULL;
    TTF_Font *font = NULL;
    std::map<std::string, SDL_Texture *> texture_cache;
    std::map<std::string, SDL_Texture *> text_cache;
    bool has_focus = true;
    float fps = -1.0;
};



#define DRAW_NORMAL 0
#define DRAW_GHOST 1
#define DRAW_DND 2
#define DRAW_ORIGIN 3



extern int R(int color);
extern int G(int color);
extern int B(int color);
extern int Alpha(int color);
extern int value_color(struct value *value);
extern std::string value_color_texture(struct value *value);
extern class element *element_from_class_id(std::string classid);

extern bool wire_factory_force_new_name;

extern class wire *json_wire_factory(json &jwire);
extern void clear_wire_factory(void);

extern void register_new_canvas(class basecanvas *canvas);
extern void unregister_canvas(class basecanvas *canvas);
extern void labelevent(SDL_Event &event, std::string *text);

extern std::string generate_semi_uuid(void);
extern std::string generate_wire_name(void);
extern void populate_library(std::string directory);

extern void queue_calculate(class element *element);
extern void remove_from_calculate_queue(class element *element);
extern void run_queued_calculations(void);

extern unsigned int SDL_timer_event;

#define EVENT_ZOOM_TO_FIT  (SDL_timer_event + 1)
#define EVENT_STOP_CLOCK   (SDL_timer_event + 2)
#define EVENT_SINGLE_CLOCK (SDL_timer_event + 3)
#define EVENT_START_CLOCK  (SDL_timer_event + 4)
#define EVENT_SAVE_TO_LIBRARY (SDL_timer_event + 5)
#define EVENT_RELOAD_ICONBAR (SDL_timer_event + 6)
#define EVENT_SAVE (SDL_timer_event + 7)
#define EVENT_RUN_VERILOG (SDL_timer_event + 8)
#define EVENT_AUTOCLOCK (SDL_timer_event + 9)
#define EVENT_CLOSE_DIALOG (SDL_timer_event + 10)
#define EVENT_SYNTH_DATA (SDL_timer_event + 11)
#define EVENT_SYNTH_DONE (SDL_timer_event + 12)
#define EVENT_PROGRAM_FPGA (SDL_timer_event + 13)
#define EVENT_REWIRE (SDL_timer_event + 14)
#define EVENT_SEQUENCER_RESET (SDL_timer_event + 15)

extern void callback_editname(class element *element);
extern SDL_Texture *IMG_LoadTextureFromMem(SDL_Renderer *renderer, const char *filename);
extern int wire_to_color(int color);
extern std::string wire_color_name(int color);
extern std::string append_random_bits(std::string input);
extern std::string path_to_template(std::string instr);
extern std::string template_to_path(std::string instr);
#endif