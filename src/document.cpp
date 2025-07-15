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
#include "document.h"
#include "model_clock.h"
#include "model_nest.h"
#include "library.h"
#include "canvas.h"
#include "scene.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL2_framerate.h>
#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include <sys/time.h>
#include <unistd.h>

#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_sdlrenderer2.h"

#include <nlohmann/json.hpp>
using json = nlohmann::json;


static std::vector<class basecanvas *> canvases;

extern void fill_png_maps(void);
extern void clear_png_maps(void);
extern void set_timer(void);
extern bool show_fps;


float tv_delta_msec(struct timeval *tv1, struct timeval *tv2)
{
	double d;

	d = tv2->tv_sec - tv1->tv_sec;
	d = d * 1000;
	d += tv2->tv_usec/ 1000.0;
	d -= tv1->tv_usec/ 1000.0;

	return d;
}

float tv_delta_usec(struct timeval *tv1, struct timeval *tv2)
{
	double d;

	d = tv2->tv_sec - tv1->tv_sec;
	d = d * 1000000;
	d += tv2->tv_usec;
	d -= tv1->tv_usec;

	return d;
}


static bool sdl_initialized = false;
void init_SDL(void)
{
	if (sdl_initialized)
		return;
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
	TTF_Init();
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
	SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);

	
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	ImGui::StyleColorsDark();

	sdl_initialized = true;
}

extern std::string current_project;
document::document(std::string _name, bool _library_mode)
{
	class canvas *_canvas;
	class scene *_scene;
	float d1,d2,d3;

	struct capabilities cap = {};


	library_mode = _library_mode;

	init_SDL();

	std::string filename;

	fill_png_maps();

	name = _name;

	current_project = name;

	if (std::filesystem::exists(name + "/verilog/Makefile") && !library_mode)
		cap.has_full_workflow = true;


	SDL_GetDisplayDPI(0, &d1, &d2, &d3);
	printf("DPI %5.2f %5.2f %5.2f\n", d1, d2, d3);

	SDL_timer_event = SDL_RegisterEvents(16);

	set_timer();

	_scene = new scene("main");

	_canvas = new canvas(_scene, &cap);

	_canvas->set_project_name(name);

	register_new_canvas(_canvas);

	filename = name;
	if (!library_mode)
		filename = filename + "/scene.json";

	if (access(filename.c_str(), R_OK) == 0) {
		json j;
		std::ifstream input(filename.c_str());
		input >> j;
		_scene->from_json(j);
	}
	clear_wire_factory();
	_scene->cycle_color();
}

void document::save_verilog(std::string path, std::string filename)
{
	if (library_mode)
		return;
	printf("Saving as %s\n", filename.c_str());
	std::ofstream output(filename);
	class scene *_scene;
	class canvas *_canvas = (class canvas *)canvases[0];
	_scene = _canvas->get_scene();

	output << _scene->get_verilog_main() << "\n" << _scene->get_verilog_modules(path);
	output.close();
}

void document::save_json(void)
{
	class canvas *_canvas = (class canvas *)canvases[0];
	class scene *_scene =  _canvas->get_scene();
	std::string filename;

	if (!library_mode)
		filename = name + "/scene.json";
	else
		filename = name;

	printf("Saving to %s \n", filename.c_str());

	json j;

	_scene->to_json(j);

	std::ofstream output(filename);

	output << j.dump(4);
	output.close();
}

document::~document(void)
{
	class canvas *_canvas = (class canvas *)canvases[0];

	if (!library_mode)
		std::filesystem::create_directory(name);

	save_json();
	if (!library_mode) {	
		std::filesystem::create_directory(name + "/verilog");
		save_verilog(name + "/verilog", name + "/verilog//main.v");
	}
	delete _canvas;

	TTF_Quit();
	SDL_Quit();
	clear_png_maps();
}

bool clock_running = true;

void document::run(void)
{
	SDL_Event event;
	int ret = 0;
	bool leave = false;
	FPSmanager fps = {};
	
	SDL_initFramerate(&fps);
	SDL_setFramerate(&fps, 30);

	for (auto canvas:canvases)
		canvas->draw();

	while (!leave) {
		run_queued_calculations();
//		ret = SDL_WaitEventTimeout(&event, 1);
		ret = SDL_PollEvent(&event);

		if (event.type == SDL_QUIT)
			leave = true;

		if (ret) {
			/* toggle the clock if there is a timer event -- this is the best race-free place to do this*/
			if (event.type == EVENT_STOP_CLOCK) {
				printf("Receive stop clock\n");
				clock_running = false;
			}
			if (event.type == EVENT_START_CLOCK) {
				printf("Receive start clock\n");
				clock_running = true;
			}
			if (event.type == EVENT_SAVE_TO_LIBRARY) {
				class model_nest *nest = (class model_nest *)event.user.data1;
				if (library_mode) 
					nest->save_to_library("");
				else {
					nest->save_to_library(name + "/library");
				}
				clear_library();
				populate_library("library/");
				populate_library(name + "/library");

				SDL_Event ev = {};

				ev.type = EVENT_RELOAD_ICONBAR;
				ev.user.code = 0;
				SDL_PushEvent(&ev);				
			}
			if (event.type == EVENT_SAVE || event.type == EVENT_RUN_VERILOG) {
				save_json();
				save_verilog(name + "/verilog", name + "/verilog//main.v");
			}
			if ((event.type == SDL_timer_event && clock_running) || event.type == EVENT_SINGLE_CLOCK) {
				global_clock.valid = true;
				global_clock.is_clock = true;
				global_clock.boolval = !global_clock.boolval;
				global_clock.arrayval++;
			}
			for (unsigned int i = 0; i < canvases.size(); i++)  {
				auto canvas = canvases[i];
				bool thisret = false;

				if (event.window.windowID == canvas->get_window_ID() || event.type >= SDL_timer_event) {
					thisret = canvas->handle_event(event);
					if (thisret) {
						if (i == 0)
							leave = true;
						break;
					}
				}

			}
		} else {
			SDL_framerateDelay(&fps);
			//			SDL_Delay(1);
		}

		if (leave)
			printf("Want to quit\n");

		run_queued_calculations();
		if (!ret && !leave) {
			struct timeval now;
			gettimeofday(&now, NULL);
			for (auto canvas: canvases) {
				if (canvas->canvas_has_focus()) {
					canvas->draw();
				} else {
					if (tv_delta_msec(&previous_slow_draw, &now) > 60)
						canvas->draw();
				}
			}
			if (tv_delta_msec(&previous_slow_draw, &now) > 60) {
				previous_slow_draw = now;
			}
			fast_draw_counter++;
			if (fast_draw_counter >= 60) {
				double fps = tv_delta_usec(&previous_fast_draw, &now) / fast_draw_counter;
				fps = fps / 1000000.0; 
				fps = 1.0 / fps;
				if (!show_fps)
					fps = -1.0;
				for (auto c : canvases)
					c->set_fps(fps);
				previous_fast_draw = now;
				fast_draw_counter = 0;
			}
			for (auto canvas: canvases) 
				canvas->present();

		}
	}

}




void register_new_canvas(class basecanvas *canvas)
{
	canvases.push_back(canvas);
}

void unregister_canvas(class basecanvas *canvas)
{
	for (unsigned int i = 0; i < canvases.size(); i++) {
		if (canvases[i] == canvas) {
			canvases.erase(canvases.begin() + i);
			return;
		}
	}
}

/* +0 == timer
   +1 == fit-to-screen
   */
unsigned int SDL_timer_event = 0;

std::string current_project = "";

/* replace project/<name/ with $project and back */
std::string path_to_template(std::string instr)
{
	std::string s = instr;
	size_t index;
	std::string find = current_project;
	index = s.find(find);
	if (index != std::string::npos && index >= 0)
		s.replace(index, find.size(), "$project"); 
	return s;
}
std::string template_to_path(std::string instr)
{
	std::string s = instr;
	size_t index;
	std::string find = "$project";
	index = s.find(find);
	if (index != std::string::npos && index >= 0)
		s.replace(index, find.size(), current_project); 
	return s;
}
