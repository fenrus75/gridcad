#include "gridcad.h"

#include "model_nest.h"
#include <sys/time.h>
#include <nlohmann/json.hpp>

model_nest::model_nest(float _X, float _Y) : element(_X,_Y, "SubScreen")
{
	sizeX = 4;
	sizeY = 4;
	X = floorf(_X);
	Y = floorf(_Y);    
	_scene = new scene();
}

model_nest::~model_nest(void)
{
}


void model_nest::drawAt(class canvas *canvas, float X, float Y, int type)
{
	if (!selected) {
		canvas->draw_image("assets/nest/nest_back.png", X, Y, sizeX, sizeY, Alpha(type));
		canvas->draw_image("assets/nest/icon.png", X+0.15, Y+0.15, sizeX-0.3, sizeY-0.3, Alpha(type), true);
	} else {
		canvas->draw_image("assets/nest/nest_selected.png", X, Y, sizeX, sizeY, Alpha(type));
	}

	if (selected && single && edit_mode) {
          struct timeval tv;
          gettimeofday(&tv, NULL);
          if (tv.tv_usec > 500000)
             canvas->draw_text(name + "|", X, Y + sizeY, sizeX, 1);
          else
             canvas->draw_text(name + " ", X, Y + sizeY, sizeX, 1);
        } else {
     		 canvas->draw_text(name + " ", X, Y + sizeY, sizeX, 1);
     	}
	
}


void model_nest::calculate(int ttl)
{
}


bool model_nest::mouse_select(float _X, float _Y)
{
	long int click = time(NULL);

	if (click - previous_click > 1) {
		previous_click = click;
		return false;
	}	
	
	if (!canvas) {
		printf("Spawning a new window\n");
		canvas = new class canvas(_scene);
		register_new_canvas(canvas);	
	} else {
		canvas->unhide();
	}
	previous_click = click;
	return false;
}


void model_nest::handle_event(SDL_Event &event)
{
    if (!selected || !single)
        return;
    switch (event.type) {
	case SDL_KEYDOWN:        
        	switch (event.key.keysym.sym) {
                case SDLK_RETURN:
                    edit_mode = !edit_mode;
                    break;
                }
                break;
        }
    if (edit_mode)
      labelevent(event, &name);
}

void model_nest::to_json(json &j)
{
     json p;
     element::to_json(j);
     _scene->to_json(p);
     j["scene"] = p;   
}
void model_nest::from_json(json &j)
{
     element::from_json(j);
     _scene->from_json(j["scene"]);
}
