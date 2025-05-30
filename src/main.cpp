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

#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>

#include "gridcad.h"
#include "document.h"

int main(int argc, char **argv)
{	
	std::string name = "scene";
	class document *document;
	
	if (argc > 1)
		name = argv[1];
	
	populate_library("library/");

	document = new class document(name);

	document->run();

	delete document;

	return 0;
}
