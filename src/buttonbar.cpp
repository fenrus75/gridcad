#include "gridcad.h"

#include "buttonbar.h"



buttonbar::buttonbar(class canvas *_canvas, unsigned int _width)
{
    canvas = _canvas;
    width = _width - 5;
}


buttonbar::~buttonbar(void)
{
    for (auto b : buttons) {
        delete b;
    }
    buttons.clear();
}

#define Y_SPACING 1.1

void buttonbar::handle_event(SDL_Event &event)
{
        unsigned int x, y;
	switch (event.type) {
		case SDL_MOUSEBUTTONDOWN:
		        unsigned int buttonindex;
                        x = event.motion.x;
                        y = event.motion.y;
                        
                        if (x > width)
                            break;
                        buttonindex = y / (Y_SPACING * width);
                        if (buttonindex >= buttons.size()) 
                            break;
                            
                        buttons[buttonindex]->activate();
                        break;
		case SDL_MOUSEBUTTONUP:
		        for (auto button : buttons)
                            button->deactivate();
        }
    
}

void buttonbar::add_button(std::string text, std::string icon, int _event)
{
    class barbutton *button = new barbutton(text, icon, _event, canvas);
    buttons.push_back(button);
}


void buttonbar::draw_at(class canvas *canvas, int W, int H)
{
    float w, h;
    
    w = canvas->scale_to_X(W);
    h = canvas->scale_to_Y(W);
    
    for (unsigned int i = 0; i < buttons.size(); i++) {
        auto button = buttons[i];
        button->draw_at(canvas, canvas->scr_to_X(0), canvas->scr_to_Y(0) + Y_SPACING * h  * i, w, h);
    }
}



barbutton::barbutton(std::string _text, std::string _icon, int _event, class canvas *_canvas)
{
    text = text;
    icon = _icon;
    eventnr = _event;
    canvas = _canvas;
}

barbutton::~barbutton(void)
{
}


void barbutton::draw_at(class canvas *canvas, float X, float Y, float W, float H)
{	
    if (active) {
        canvas->draw_image("assets/lightgray.png", X,Y,W,H);
        X = X + canvas->scale_to_X(5);
        Y = Y + canvas->scale_to_Y(5);;
    }
    canvas->draw_image(icon, X,Y,W,H);
}

void barbutton::activate(void)
{
	SDL_Event ev = {};


	ev.type = eventnr;
	ev.user.code = 0;
	ev.user.data1 = canvas->get_scene();
	SDL_PushEvent(&ev);
	active = true;
} 