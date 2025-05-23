#include "gridcad.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>

document::document(std::string _name)
{
	class canvas *_canvas;
	class scene *_scene;

	name = _name;

	SDL_Init(SDL_INIT_VIDEO);
	TTF_Init();
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");

	_scene = new scene();

	_canvas = new canvas(_scene);

	canvases.push_back(_canvas);

	if (access((name + ".json").c_str(), R_OK) == 0) {
		json j;
		std::ifstream input((name + ".json").c_str());
		input >> j;
		_scene->from_json(j);
	}

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
		ret = SDL_PollEvent(&event);

		if (event.type == SDL_QUIT)
			leave = true;

		if (ret) {
			for (auto canvas:	canvases) {
				bool thisret = false;
				if (event.window.windowID == canvas->get_window_ID()) {
					thisret = canvas->handleEvent(event);
					if (thisret) {
						printf("Want to quit now\n");
						leave = true;
					}
				}

			}
		}

		if (leave)
			printf("Want to quit\n");

		if (!ret && !leave) {
			for (auto canvas:	canvases)
				canvas->draw();
		}
	}

}
