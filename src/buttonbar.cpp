#include "gridcad.h"
#include "buttonbar.h"



buttonbar::buttonbar(class basecanvas *_canvas, unsigned int _width)
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

#define Y_SPACING 1.2

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
			break;
		case SDL_MOUSEMOTION:
			mouseX = event.motion.x;
			mouseY = event.motion.y;

	}

}

void buttonbar::add_button(std::string text, std::string icon, int _event)
{
	class barbutton *button = new barbutton(text, icon, _event, canvas);
	buttons.push_back(button);
}

extern float ratio_effect(float ratio);
static float delta_hover(uint64_t deltaT)
{
	float f;
	if (deltaT > 300)
		return 0;
		
	f = ratio_effect(deltaT/300.0);
	f = (1 - (0.8 + 0.2 * f))/2;
	return f;
	
}


void buttonbar::draw_at(class basecanvas *canvas, int W, int H)
{
	float w, h;
	unsigned int buttonindex;

	if (mouseX > width)
		buttonindex = 600;
	else
		buttonindex = mouseY / (Y_SPACING * width);

	w = canvas->scale_to_X(W);
	h = canvas->scale_to_Y(W);
	
	if (buttonindex != hover_button) {
		hover_button = buttonindex;
		hover_time = SDL_GetTicks64();
	}
	

	for (unsigned int i = 0; i < buttons.size(); i++) {
		auto button = buttons[i];
		float delta = 0;
		if (hover_button == i)
			delta = delta_hover(SDL_GetTicks64() - hover_time);
		button->draw_at(canvas, canvas->scr_to_X(0), canvas->scr_to_Y(0) + Y_SPACING * h  * i, w, h, buttonindex == i, delta);
	}

	if (fps > 0) {
		char buf[128];
		std::string s;
		sprintf(buf, "%3.1ffps", fps);
		s = buf;
		canvas->draw_text(s, canvas->scr_to_X(0), canvas->scr_to_Y(0) + Y_SPACING * h  * buttons.size() + 5, w, h);
	}


}

std::string buttonbar::current_tooltip(unsigned int x, unsigned int y)
{
	unsigned int buttonindex;

	if (mouseX > width)
		return "";
	buttonindex = mouseY / (Y_SPACING * width);
	if (buttonindex >= buttons.size()) 
		return "";

	return buttons[buttonindex]->get_tool_tip();
}


barbutton::barbutton(std::string _text, std::string _icon, int _event, class basecanvas *_canvas)
{
	text = _text;
	icon = _icon;
	eventnr = _event;
	canvas = _canvas;
}

barbutton::~barbutton(void)
{
}


void barbutton::draw_at(class basecanvas *canvas, float X, float Y, float W, float H, bool hover, float delta)
{	
	if (!active && hover) 
		canvas->draw_image("assets/lightgray.png", X - delta,Y - delta, W + 2 * delta, H + 2 * delta);

	if (active) {
		canvas->draw_image("assets/lightgray.png", X,Y,W,H);
		X = X + canvas->scale_to_X(5);
		Y = Y + canvas->scale_to_Y(5);;
	}
	canvas->draw_image(icon, X - delta, Y - delta ,W + 2 * delta, H + 2 * delta);
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
