#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>

#include "gridcad.h"

int main(int argc, char **argv)
{
	class document *document;

	document = new class document("scene");

	document->run();

	delete document;

	return 0;
}
