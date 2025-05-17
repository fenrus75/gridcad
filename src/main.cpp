#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <iostream>
#include <fstream>
#include <string>

#include "gridcad.h"
#include "models.h"

int main(int argc, char **argv)
{
    class canvas *_canvas;
    class scene *_scene;
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");

    _scene = new scene();
    
    _canvas = new canvas(_scene);
    
    {
        json j;
        std::ifstream input("scene.json");
        input >> j;
        _scene->from_json(j);
    }
    
    
    _canvas->eventloop();
    

    {    
        json j;
    
        _scene->to_json(j);
    
        std::ofstream output("scene.json");
    
        output << j.dump(4);
        output.close();
    }

    delete _canvas;    
    delete _scene;
    

    TTF_Quit();    
    SDL_Quit();
    return 0;
}
