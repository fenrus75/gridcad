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

#include <execinfo.h>
#include <unistd.h>
#include <signal.h>

#include "gridcad.h"
#include "document.h"

static unsigned char signalstack[2 * 1024 * 1024];

void segv_handler(int sig) {
	void *entries[32];
	size_t count;
	
	count = backtrace(entries, 32);
	fprintf(stderr, "Segmentation fault:\n");
	backtrace_symbols_fd(entries, count, STDERR_FILENO);
	exit(1);
}

int main(int argc, char **argv)
{	
	std::string name = "default";
	class document *document;

	stack_t stack = {};
	struct sigaction segv = {};
	
	stack.ss_sp = &signalstack;
	stack.ss_size = sizeof(signalstack);
	stack.ss_flags = 0;
	
	sigaltstack(&stack, NULL);
	
	segv.sa_handler = &segv_handler;
	segv.sa_flags = SA_ONSTACK;
	
	
	sigaction(SIGSEGV, &segv, NULL);
	
	if (argc > 1)
		name = argv[1];
	
	populate_library("library/");

	name = "projects/" + name;
	
	/* project based library */
	populate_library(name + "/library");

	document = new class document(name);

	document->run();
	
	delete document;
	document = NULL;

	return 0;
}
