#include "gridcad.h"
#include "name.h"

#include <sys/time.h>

name::name(std::string *_val)
{
	value = _val;
	cursorpos = value->size();
}

name::~name(void)
{
}

float name::relative_cursor_pos(class basecanvas *canvas, std::string text, float H)
{
	SDL_Point size;
	float w,h,W = 40;

	if (text == "")
		return 0;

	SDL_Texture *texture = canvas->text_to_texture(text);
	SDL_QueryTexture(texture, NULL, NULL, &size.x, &size.y);

	h = H;
	w = 1.0 * size.x / size.y * h;
	if (w > W) {
		w = W;
		h = H * size.y / size.x * w;
	}
	return w;
}


void name::drawAt(class basecanvas *canvas, float X, float Y, float W, float H)
{
	if (edit_mode) {
		struct timeval tv;
		gettimeofday(&tv, NULL);
		std::string before, after;
		if (cursorpos < value->size()) {
			before = value->substr(0, cursorpos);
			after = value->substr(cursorpos, std::string::npos);
		} else {
			before = *value;
			after = "";
		};
		if (tv.tv_usec > 500000)
			canvas->draw_image("assets/lightgray.png", X + relative_cursor_pos(canvas, before, H), Y, 0.2, H);
		canvas->draw_text_left(*value, X, Y, W + 5, H);

		if (tv.tv_usec > 500000)
			canvas->draw_text_left("|", X + relative_cursor_pos(canvas, before, H), Y, W + 5, H);

	} else {
		canvas->draw_text(*value + " ", X, Y, W, H);
	}
}

void name::handle_event(SDL_Event &event)
{
	char c;
	switch (event.type) {
		case SDL_KEYDOWN:
			switch (event.key.keysym.sym) {
				case SDLK_BACKSPACE:
					if (edit_mode)
						backspace();
					break;    
				case SDLK_0:
				case SDLK_1:
				case SDLK_2:
				case SDLK_3:
				case SDLK_4:
				case SDLK_5:
				case SDLK_6:
				case SDLK_7:
				case SDLK_8:
				case SDLK_9:                
					c = '0';
					c += event.key.keysym.sym - SDLK_0;
					if (edit_mode)
						insert_char(c);
					break;
				case SDLK_UNDERSCORE:                
					c = '_';
					if (edit_mode)
						insert_char(c);
					break;
				case SDLK_PLUS:   
				case SDLK_KP_PLUS:             
					c = '+';
					if (edit_mode)
						insert_char(c);
					break;
				case SDLK_MINUS:                
				case SDLK_KP_MINUS:
					c = '-';
					if (event.key.keysym.mod & (KMOD_LSHIFT))
						c = '_';
					if (edit_mode)
						insert_char(c);
					break;
				case SDLK_EQUALS:
					c= '=';
					if (event.key.keysym.mod & (KMOD_LSHIFT))
						c = '+';
					if (edit_mode)
						insert_char(c);
					break;                
				case SDLK_a:
				case SDLK_b:
				case SDLK_c:
				case SDLK_d:
				case SDLK_e:
				case SDLK_f:
				case SDLK_g:
				case SDLK_h:
				case SDLK_i:
				case SDLK_j:                
				case SDLK_k:
				case SDLK_l:
				case SDLK_m:
				case SDLK_n:
				case SDLK_o:
				case SDLK_p:
				case SDLK_q:
				case SDLK_r:
				case SDLK_s:
				case SDLK_t:                
				case SDLK_u:
				case SDLK_v:
				case SDLK_w:
				case SDLK_x:
				case SDLK_y:
				case SDLK_z:
					if (numbers_only)
						break;
					if (event.key.keysym.mod & (KMOD_LCTRL))
						break;
					if (event.key.keysym.mod & (KMOD_RCTRL))
						break;
					c= 'a';
					if (event.key.keysym.mod & (KMOD_LSHIFT))
						c = 'A';
					c += event.key.keysym.sym - SDLK_a;
					if (edit_mode)
						insert_char(c);
					break;
				case SDLK_SPACE:
					if (!spaces)
						break;
					if (edit_mode)
						insert_char(' ');
					break;
				case SDLK_RIGHT:
					if (!edit_mode)
						break;
					cursorpos++;
					if (cursorpos >= value->size())
						cursorpos = value->size();
					break;
				case SDLK_LEFT:
					if (!edit_mode)
						break;
					if (cursorpos > 0)
						cursorpos--;
					break;
				case SDLK_RETURN:
					toggle_edit_mode();
					break;
			}
			break;
	}
}

void name::insert_char(unsigned char c)
{
	std::string before, after;
	char buffer[2];


	if (cursorpos >= value->size()) {
		*value += c;
		cursorpos++;
		return;
	}
	before = value->substr(0, cursorpos);
	after = value->substr(cursorpos, std::string::npos);
	buffer[0] = c;
	buffer[1] = 0;
	after = before + buffer + after;
	*value = after;
	cursorpos++;
}

void name::backspace(void)
{
	std::string before, after;


	if (value->size() == 0 || cursorpos == 0)
		return;

	if (cursorpos >= value->size()) {
		value->pop_back();
		cursorpos++;
		return;
	}
	before = value->substr(0, cursorpos - 1);
	after = value->substr(cursorpos, std::string::npos);
	after = before + after;
	*value = after;
	cursorpos--;
}



void name::update_string_pointer(std::string *ptr)
{
	value = ptr;
}
