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

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>

#include <nlohmann/json.hpp>
using json = nlohmann::json;


static std::vector<class basecanvas *> canvases;

document::document(std::string _name)
{
	class canvas *_canvas;
	class scene *_scene;

	name = _name;

	SDL_Init(SDL_INIT_VIDEO);
	SDL_Init(SDL_INIT_TIMER);
	TTF_Init();
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");
	
	SDL_timer_event = SDL_RegisterEvents(1);

	_scene = new scene("main");

	_canvas = new canvas(_scene);

	register_new_canvas(_canvas);
	
	if (access((name + ".json").c_str(), R_OK) == 0) {
		json j;
		std::ifstream input((name + ".json").c_str());
		input >> j;
		_scene->from_json(j);
	}
	clear_wire_factory();
}

document::~document(void)
{
	class scene *_scene;
	class canvas *_canvas = (class canvas *)canvases[0];

	_scene = _canvas->get_scene();

	json j;

	_scene->to_json(j);

	std::ofstream output((name + ".json").c_str());

	output << j.dump(4);
	output.close();
	delete _canvas;

	TTF_Quit();
	SDL_Quit();
}

void document::run(void)
{
	SDL_Event event;
	int ret = 0;
	bool leave = false;

	for (auto canvas:canvases)
		canvas->draw();

	while (!leave) {
//		ret = SDL_WaitEventTimeout(&event, 1);
		ret = SDL_PollEvent(&event);

		if (event.type == SDL_QUIT)
			leave = true;

		if (ret) {
			for (unsigned int i = 0; i < canvases.size(); i++)  {
				auto canvas = canvases[i];
				bool thisret = false;
				if (event.window.windowID == canvas->get_window_ID() || event.type == SDL_timer_event) {
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

		if (!ret && !leave) {
			for (auto canvas: canvases)
				canvas->draw();
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

unsigned int SDL_timer_event = 0;
