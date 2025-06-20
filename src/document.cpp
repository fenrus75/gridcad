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

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include <sys/time.h>
#include <unistd.h>

#include <nlohmann/json.hpp>
using json = nlohmann::json;


static std::vector<class basecanvas *> canvases;

extern void fill_png_maps(void);
extern void set_timer(void);


float tv_delta_msec(struct timeval *tv1, struct timeval *tv2)
{
	double d;
	
	d = tv2->tv_sec - tv1->tv_sec;
	d = d * 1000;
	d += tv2->tv_usec/ 1000.0;
	d -= tv1->tv_usec/ 1000.0;
	
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
	
	sdl_initialized = true;
}

extern std::string current_project;
document::document(std::string _name)
{
	class canvas *_canvas;
	class scene *_scene;
	float d1,d2,d3;
	
	struct capabilities cap = {};
	
	init_SDL();
	
	std::string filename;
	
	fill_png_maps();

	name = _name;
	
	current_project = name;
	
	if (std::filesystem::exists(name + "/verilog/Makefile"))
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
	
	filename = name + "/scene.json";
	
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

	std::filesystem::create_directory(name);

	save_json();	
	std::filesystem::create_directory(name + "/verilog");
	save_verilog(name + "/verilog", name + "/verilog//main.v");
	delete _canvas;

	TTF_Quit();
	SDL_Quit();
}

bool clock_running = true;

void document::run(void)
{
	SDL_Event event;
	int ret = 0;
	bool leave = false;

	for (auto canvas:canvases)
		canvas->draw();

	while (!leave) {
//		ret = SDL_WaitEventTimeout(&event, 1);
		run_queued_calculations();
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
				nest->save_to_library(name + "/library");
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
//			SDL_sleep(1);
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
					if (tv_delta_msec(&previous_draw, &now) > 60)
						canvas->draw();
				}
			}
			if (tv_delta_msec(&previous_draw, &now) > 60) {
				previous_draw = now;
			}
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
