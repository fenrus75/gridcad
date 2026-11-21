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
#include "verilog_import.h"
#include "vcd_session.h"
#include "placer.h"
#include "layout_overlay.h"

std::chrono::steady_clock::time_point start_time;
std::chrono::steady_clock::time_point exit_time;
bool has_exit_time = false;

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
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
extern Uint32 SDL_timer_func(Uint32 interval, void *param);
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


namespace sdl {

Context::Context()
{
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
	TTF_Init();
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	ImGui::StyleColorsDark();
}

Context::~Context()
{
	ImGui::DestroyContext();
	TTF_Quit();
	SDL_Quit();
}

} // namespace sdl

extern std::string current_project;
document::document(std::string _name, bool _library_mode,
                   std::string import_json_path,
                   std::string vcd_path,
                   std::string vcd_scope)
{
	float d1,d2,d3;

	struct capabilities cap = {};


	library_mode = _library_mode;
	import_mode = !import_json_path.empty();
	set_import_view_mode(import_mode);

	std::string filename;

	fill_png_maps();

	name = _name;

	current_project = name;

	if (std::filesystem::exists(name + "/verilog/Makefile") && !library_mode && !import_mode)
		cap.has_full_workflow = true;


	SDL_GetDisplayDPI(0, &d1, &d2, &d3);
	logger::get().info("DPI {:5.2f} {:5.2f} {:5.2f}", d1, d2, d3);

	SDL_timer_event = SDL_RegisterEvents(32);

	clock_timer = sdl::Timer(500, SDL_timer_func, nullptr);

	std::unique_ptr<scene> _scene;
	if (import_mode) {
		auto res = verilog_importer::import_file(import_json_path);
		if (res.ok && res.imported_scene) {
			logger::get().info(
			    "Imported '{}' (top '{}'): {} ports, {} consts, {} components, {} submodules, {} wires, {} warnings",
			    import_json_path, res.top_module, res.stats.ports, res.stats.constants,
			    res.stats.components, res.stats.submodules, res.stats.wires,
			    res.stats.warnings.size());
			for (const auto &w : res.stats.warnings)
				logger::get().warn("import: {}", w);
			_scene = std::move(res.imported_scene);
		} else {
			logger::get().error("Failed to import '{}': {}", import_json_path, res.error);
			_scene = std::make_unique<scene>("main");
		}
	} else {
		_scene = std::make_unique<scene>("main");
	}
	class scene *scene_ptr = _scene.get();

	main_canvas = std::make_unique<canvas>(std::move(_scene), &cap);

	main_canvas->set_project_name(name);

	register_new_canvas(main_canvas.get());

	if (!import_mode) {
		filename = name;
		if (!library_mode)
			filename = filename + "/scene.json";

		if (access(filename.c_str(), R_OK) == 0) {
			json j;
			std::ifstream input(filename.c_str());
			input >> j;
			scene_ptr->from_json(j);
		}
	}
	clear_wire_factory();
	if (!import_mode)
		scene_ptr->cycle_color();

	if (!vcd_path.empty()) {
		vcd = std::make_unique<vcd_session>();
		std::string vcd_err;
		if (vcd->load(vcd_path, vcd_err)) {
			size_t bound = vcd->bind_scene(scene_ptr, vcd_scope);
			logger::get().info(
			    "Loaded VCD '{}' (clock '{}', {} ticks, scope '{}', {} bound signals)",
			    vcd_path, vcd->get_clock_signal(), vcd->get_total_ticks(),
			    vcd->get_scope_prefix(), bound);
			set_active_vcd_session(vcd.get());
			main_canvas->set_vcd_session(vcd.get());
			vcd->push_tick_events(0);
		} else {
			logger::get().error("Failed to load VCD '{}': {}", vcd_path, vcd_err);
			vcd.reset();
		}
	}

	if (import_mode) {
		SDL_Event ev = {};
		ev.type = EVENT_ZOOM_TO_FIT;
		ev.user.code = 0;
		ev.user.data1 = scene_ptr;
		SDL_PushEvent(&ev);
	}
}

void document::save_verilog(std::string_view path, std::string_view filename)
{
	if (library_mode)
		return;
	logger::get().info("Saving as {}", filename);
	std::ofstream output{std::string(filename)};
	class scene *_scene;
	class canvas *_canvas = (class canvas *)canvases[0];
	_scene = _canvas->get_scene();

	output << _scene->get_verilog_main() << "\n" << _scene->get_verilog_modules(std::string(path));
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

	logger::get().info("Saving to {}", filename);

	json j;

	_scene->to_json(j);

	std::ofstream output(filename);

	output << j.dump(4);
	output.close();
}

document::~document(void)
{
	clock_timer.reset();
	set_active_vcd_session(nullptr);
	set_import_view_mode(false);

	/* Drain any pending VCD wire update events so their payloads are freed cleanly. */
	SDL_Event ev;
	while (SDL_PeepEvents(&ev, 1, SDL_GETEVENT, EVENT_VCD_WIRE_UPDATE, EVENT_VCD_WIRE_UPDATE) > 0) {
		std::unique_ptr<vcd_wire_update> upd(static_cast<vcd_wire_update *>(ev.user.data1));
	}

	if (!import_mode) {
		if (!library_mode)
			std::filesystem::create_directory(name);

		save_json();
		if (!library_mode) {
			std::filesystem::create_directory(name + "/verilog");
			save_verilog(name + "/verilog", name + "/verilog//main.v");
		}
	} else if (main_canvas && main_canvas->get_scene()) {
		layout_overlay::save_dirty_recursive(main_canvas->get_scene());
	}
	clear_png_maps();
}

bool clock_running = true;

static struct timeval previous_frame = {};

static void wait_fps(float target_fps)
{
	struct timeval now;
	double d = 0.0;
	double target_time = 1.0 / target_fps;
	double time_left;
	
	gettimeofday(&now, nullptr);
	d = now.tv_sec - previous_frame.tv_sec + now.tv_usec/1000000.0 -  previous_frame.tv_usec/1000000.0;
	
	time_left = target_time - d;
	if (time_left < 0.0002)  /* skip a whole frame */
		time_left += target_time;
	if (time_left < 0.0002) {
		gettimeofday(&previous_frame, nullptr);
		return;
	}
	usleep(time_left * 1000000 - 200);
	gettimeofday(&previous_frame, nullptr);
}

void document::run(void)
{
	SDL_Event event;
	int ret = 0;
	bool leave = false;
	
	for (auto canvas:canvases)
		canvas->draw();

	while (!leave) {
		if (has_exit_time && std::chrono::steady_clock::now() >= exit_time) {
			logger::get().info("Exiting due to exit-after timeout");
			leave = true;
			break;
		}
		run_queued_calculations();
//		ret = SDL_WaitEventTimeout(&event, 1);
		ret = SDL_PollEvent(&event);

		if (event.type == SDL_QUIT)
			leave = true;

		if (ret) {
			/* toggle the clock if there is a timer event -- this is the best race-free place to do this*/
			if (event.type == EVENT_STOP_CLOCK) {
				logger::get().debug("Receive stop clock");
				clock_running = false;
			}
			if (event.type == EVENT_START_CLOCK) {
				logger::get().debug("Receive start clock");
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
				if (import_mode) {
					if (main_canvas && main_canvas->get_scene())
						layout_overlay::save_dirty_recursive(main_canvas->get_scene());
				} else {
					save_json();
					save_verilog(name + "/verilog", name + "/verilog//main.v");
				}
			}
			if (event.type == EVENT_VCD_WIRE_UPDATE) {
				std::unique_ptr<vcd_wire_update> upd(
				    static_cast<vcd_wire_update *>(event.user.data1));
				if (upd && main_canvas && main_canvas->get_scene()) {
					main_canvas->get_scene()->broadcast_vcd_value(
					    upd->vcd_name, upd->val);
				}
			}
			if (event.type == EVENT_VCD_SEEK && vcd && vcd->is_loaded() &&
			    vcd->get_total_ticks() > 0) {
				size_t target = (event.user.code >= 0)
				                    ? static_cast<size_t>(event.user.code)
				                    : 0;
				vcd->push_tick_events(target);
			}
			if (event.type == EVENT_VCD_UNLOCK && main_canvas && main_canvas->get_scene()) {
				clock_running = false;
				if (vcd)
					vcd->set_unlocked(true);
				main_canvas->get_scene()->unlock_all_vcd();
				logger::get().info("VCD wires unlocked; free simulation active");
			}
			if (vcd && vcd->is_loaded() && event.type == SDL_KEYDOWN) {
				if (event.key.keysym.sym == SDLK_RIGHTBRACKET) {
					size_t next = (vcd->get_current_tick() + 1 < vcd->get_total_ticks())
					                  ? (vcd->get_current_tick() + 1)
					                  : 0;
					vcd->push_tick_events(next);
					logger::get().info("VCD tick {}/{} (t={})",
					                   vcd->get_current_tick(), vcd->get_total_ticks(),
					                   vcd->get_current_timestamp());
				} else if (event.key.keysym.sym == SDLK_LEFTBRACKET) {
					size_t prev = (vcd->get_current_tick() > 0)
					                  ? (vcd->get_current_tick() - 1)
					                  : (vcd->get_total_ticks() > 0 ? vcd->get_total_ticks() - 1 : 0);
					vcd->push_tick_events(prev);
					logger::get().info("VCD tick {}/{} (t={})",
					                   vcd->get_current_tick(), vcd->get_total_ticks(),
					                   vcd->get_current_timestamp());
				}
			}
			if ((event.type == SDL_timer_event && clock_running) || event.type == EVENT_SINGLE_CLOCK) {
				if (vcd && vcd->is_loaded() && vcd->get_total_ticks() > 0) {
					size_t next = (vcd->get_current_tick() + 1 < vcd->get_total_ticks())
					                  ? (vcd->get_current_tick() + 1)
					                  : 0;
					vcd->push_tick_events(next);
				} else {
					global_clock.valid = true;
					global_clock.is_clock = true;
					global_clock.boolval = !global_clock.boolval;
					global_clock.arrayval++;
				}
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
			//			SDL_Delay(1);
		}

		if (leave)
			logger::get().debug("Want to quit");

		run_queued_calculations();
		if (!ret && !leave) {
			struct timeval now;
			gettimeofday(&now, nullptr);
			for (auto canvas: canvases) {
				if (canvas->canvas_has_focus()) {
					canvas->draw();
				} else {
					if (tv_delta_msec(&previous_slow_draw, &now) > 500)
						canvas->draw();
				}
			}
			if (tv_delta_msec(&previous_slow_draw, &now) > 500) {
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
			wait_fps(30);
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
std::string path_to_template(std::string_view instr)
{
	std::string s(instr);
	size_t index = s.find(current_project);
	if (index != std::string::npos)
		s.replace(index, current_project.size(), "$project"); 
	return s;
}
std::string template_to_path(std::string_view instr)
{
	std::string s(instr);
	constexpr std::string_view find = "$project";
	size_t index = s.find(find);
	if (index != std::string::npos)
		s.replace(index, find.size(), current_project); 
	return s;
}
