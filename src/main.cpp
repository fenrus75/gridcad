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

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <charconv>
#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>

#include "gridcad.h"
#include "crash_handler.h"
#include "document.h"
#include "project.h"

int main(int argc, char **argv)
{	
	start_time = std::chrono::steady_clock::now();
	std::string name = "default";
	std::string import_json_path = "";
	std::string vcd_path = "";
	std::string vcd_scope = "";
	bool library_mode = false;

	crash_handler::install_fallback_handler();

	std::vector<std::string> positionals;
	for (int i = 1; i < argc; ++i) {
		std::string arg = argv[i];
		if ((arg == "-l" || arg == "--logfile") && i + 1 < argc) {
			logger::get().set_log_file(argv[++i]);
		} else if (arg == "--loglevel" && i + 1 < argc) {
			std::string level = argv[++i];
			if (level == "debug") logger::get().set_level(log_level::debug);
			else if (level == "info") logger::get().set_level(log_level::info);
			else if (level == "warn") logger::get().set_level(log_level::warning);
			else if (level == "error") logger::get().set_level(log_level::error);
		} else if (arg == "--exit-after" && i + 1 < argc) {
			/* Untrusted CLI input: parse non-negative timeout safely. */
			std::string_view val = argv[++i];
			int seconds = 1;
			auto [ptr, ec] = std::from_chars(val.data(), val.data() + val.size(), seconds);
			if (ec != std::errc{} || ptr != val.data() + val.size() || seconds < 0)
				seconds = 1;
			exit_time = start_time + std::chrono::seconds(seconds);
			has_exit_time = true;
		} else if (arg == "--exit-immediately") {
			/* Optional timeout in seconds (default: 1s). */
			int seconds = 1;
			if (i + 1 < argc) {
				std::string_view next = argv[i + 1];
				int parsed = 0;
				auto [ptr, ec] = std::from_chars(next.data(), next.data() + next.size(), parsed);
				if (ec == std::errc{} && ptr == next.data() + next.size() && parsed >= 0) {
					seconds = parsed;
					++i;
				}
			}
			exit_time = start_time + std::chrono::seconds(seconds);
			has_exit_time = true;
		} else if (arg == "--import" && i + 1 < argc) {
			/* Untrusted CLI path to a Turbostar structural JSON netlist. */
			import_json_path = argv[++i];
		} else if (arg == "--vcd" && i + 1 < argc) {
			/* Untrusted CLI path to a VCD waveform file. */
			vcd_path = argv[++i];
		} else if (arg == "--vcd-scope" && i + 1 < argc) {
			/* Untrusted CLI VCD scope prefix override (e.g. "top_tb.dut"). */
			vcd_scope = argv[++i];
		} else {
			positionals.push_back(arg);
		}
	}
	
	sdl::Context sdl_ctx;

	if (!import_json_path.empty()) {
		name = "import";
	} else if (!positionals.empty()) {
		name = positionals[0];
		
		if (name == "--library") {
			logger::get().info("Library mode");
			library_mode = true;
			projcanvas proj(library_mode);
		
			proj.run();
			name = proj.get_name();
		}
	} else {
		projcanvas proj;
		
		proj.run();
		name = proj.get_name();
	}
	
	logger::get().info("Starting project {}", name);
	
	populate_library("library/");

	if (!library_mode && import_json_path.empty()) {	
		name = "projects/" + name;
		/* project based library */
		populate_library(name + "/library");
	}

	document doc(name, library_mode, import_json_path, vcd_path, vcd_scope);

	doc.run();

	return 0;
}
