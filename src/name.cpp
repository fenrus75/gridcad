#include "gridcad.h"
#include "name.h"

#include <sys/time.h>

name::name(std::string *_val)
{
	value = _val;
	cursorpos = value->size();
}

float name::relative_cursor_pos(class basecanvas *canvas, std::string_view text, float H)
{
	float tw = 0.0f, th = 0.0f;
	float w,h,W = 40;

	if (text == "")
		return 0;

	SDL_Texture *texture = canvas->text_to_texture(text);
	SDL_GetTextureSize(texture, &tw, &th);

	h = H;
	w = tw / th * h;
	if (w > W) {
		w = W;
		h = H * th / tw * w;
	}
	return w;
}


void name::drawAt(class basecanvas *canvas, float X, float Y, float W, float H)
{
	if (edit_mode) {
		struct timeval tv;
		gettimeofday(&tv, nullptr);
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
		case SDL_EVENT_KEY_DOWN:
			switch (event.key.key) {
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
					c += event.key.key - SDLK_0;
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
					if (event.key.mod & (SDL_KMOD_LSHIFT))
						c = '_';
					if (edit_mode)
						insert_char(c);
					break;
				case SDLK_EQUALS:
					c= '=';
					if (event.key.mod & (SDL_KMOD_LSHIFT))
						c = '+';
					if (edit_mode)
						insert_char(c);
					break;                
				case SDLK_A:
				case SDLK_B:
				case SDLK_C:
				case SDLK_D:
				case SDLK_E:
				case SDLK_F:
				case SDLK_G:
				case SDLK_H:
				case SDLK_I:
				case SDLK_J:                
				case SDLK_K:
				case SDLK_L:
				case SDLK_M:
				case SDLK_N:
				case SDLK_O:
				case SDLK_P:
				case SDLK_Q:
				case SDLK_R:
				case SDLK_S:
				case SDLK_T:                
				case SDLK_U:
				case SDLK_V:
				case SDLK_W:
				case SDLK_X:
				case SDLK_Y:
				case SDLK_Z:
					if (numbers_only)
						break;
					if (event.key.mod & (SDL_KMOD_LCTRL))
						break;
					if (event.key.mod & (SDL_KMOD_RCTRL))
						break;
					c= 'a';
					if (event.key.mod & (SDL_KMOD_LSHIFT))
						c = 'A';
					c += event.key.key - SDLK_A;
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
					set_edit_mode(false);
					break;
			}
			break;
	}
}

void name::insert_char(unsigned char c)
{
	if (cursorpos >= value->size()) {
		*value += c;
		cursorpos++;
		return;
	}
	value->insert(cursorpos, 1, c);
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
