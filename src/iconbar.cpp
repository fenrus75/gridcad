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

#include "gridcad.h"
#include "element.h"
#include "iconbar.h"
#include "model_nest.h"
#include "contextmenu.h"

#include <map>
#include <SDL2/SDL_image.h>


void callback_ib(class iconbar *bar, class oneiconbar *choice)
{
	bar->set(choice);
}

iconbar::iconbar(SDL_Renderer *_renderer, SDL_Rect _rect, class basecanvas *_canvas)
{
	renderer = _renderer;
	rect = _rect;
	canvas = _canvas;

	create_menu();    
}

void oneiconbar::set_element(unsigned int X, unsigned int Y, std::string class_id, std::string filename, std::string tooltip, struct library_block *block)
{
	icons[X][Y] = std::make_unique<icon>(renderer, class_id, filename, tooltip);
	cIndex = Y * 2 + X + 1;

	if (Y >= icons[X].size())
		assert(0);
	if (class_id == "library")
		icons[X][Y]->assign_library_element(*block);
}

void oneiconbar::set_element(std::string class_id, struct library_block *block)
{
	set_element(cIndex % 2, cIndex/2, class_id, "", "",block);
}
void oneiconbar::set_element(std::string class_id, std::string filename, std::string tooltip, struct library_block *block)
{
	set_element(cIndex % 2, cIndex/2, class_id, filename, tooltip, block);
}

void oneiconbar::insert_blank(void)
{
	cIndex++;
}


void iconbar::resize(SDL_Rect _rect)
{
	rect = _rect;
	for (auto &one : icons)
		one->resize(_rect);
}


void iconbar::draw(void)
{
	SDL_FRect drop;
	SDL_RenderSetClipRect(renderer, &rect);    
	float w = 0.0f, h = 0.0f;
	float newH;

	if (!dropdown)
		dropdown = IMG_LoadTextureFromMem(renderer, "assets/icondropdown.png");     

	if (current_icons)
		current_icons->draw();

	if (!dropdown)
		return;

	drop.x = static_cast<float>(rect.x + 5);
	drop.w = static_cast<float>(rect.w - 10);
	drop.y = 5.0f;
	SDL_GetTextureSize(dropdown.get(), &w, &h);

	newH = 1.0f * h / w * drop.w;
	drop.h = newH;

	SDL_SetTextureAlphaMod(dropdown.get(), 255);
	SDL_RenderCopyF(renderer, dropdown.get(), nullptr, &drop);
}

class icon * iconbar::current_icon(int ScreenX, int ScreenY)
{
	return current_icons->current_icon(ScreenX, ScreenY);
}

class contextmenu * iconbar::get_menu(int ScreenX, int ScreenY)
{
	const float offset_y = 0.5f * std::max(16.0f, static_cast<float>(rect.w) / 5.5f);
	if (ScreenY < offset_y * 1.5f) {
		return menu.get();
	}
	return nullptr;
}

std::string iconbar::current_tooltip(int ScreenX, int ScreenY)
{
	return current_icons->current_tooltip(ScreenX, ScreenY);
}

void iconbar::handle_event(SDL_Event &event)
{
	current_icons->handle_event(event);
	return;
}

void oneiconbar::handle_event(SDL_Event &event)
{
	float ScreenX = (event.motion.x);
	float ScreenY = (event.motion.y);
	
	if (event.type != SDL_MOUSEMOTION)
		return;

	float dX, dY;
	int gridX, gridY;
	const float scale = cell_scale();
	const float offset_y = top_offset();

	ScreenX -= rect.x;
	ScreenY -= rect.y;

	/* subtract the border */
	ScreenX -= 0.5f * scale;
	ScreenY -= 0.5f * scale;

	if (ScreenX < 0 || ScreenY <0) {/* left of the icons */
		hover_item_x = 8888;
		hover_item_y = 8888;
		return;
	}

	dX = ScreenX / (2.5f * scale);
	dY = (ScreenY - offset_y) / (2.5f * scale);

	gridX = floor(dX);
	gridY = floor(dY);
	
	if (gridX < 0 || gridY < 0 || gridX >= (int)icons.size() || gridY >= (int)icons[gridX].size() ) {
		hover_item_x = 8888;
		hover_item_y = 8888;
		return;
	}

	if (gridX != hover_item_x || gridY != hover_item_y) {
		hover_item_x = gridX;
		hover_item_y = gridY;
		hover_time = SDL_GetTicks64();
	}	
}


icon::icon(SDL_Renderer *renderer, std::string _class_id, std::string filename, std::string _tooltip)
{
	_renderer = renderer;
	class_id = _class_id;
	tooltip = _tooltip;
	active = false;
	if (filename != "")
		texture = IMG_LoadTextureFromMem(renderer, filename.c_str());
}

void icon::assign_library_element(struct library_block block) 
{ 
	lib = block;
	texture = IMG_LoadTextureFromMem(_renderer, lib.icon.c_str() );
	tooltip = block.tooltip;
	overlay = block.overlay;
};


/* X and Y are our box to draw in -- we'll draw centered to that */
void icon::draw(SDL_Renderer *renderer, float X, float Y, float width, float height, class basecanvas *canvas, float delta)
{
	SDL_FRect rect;
	const int color = COLOR_WIRE_SOLID;
	float w = 0.0f, h = 0.0f;
	float oX = 0.0f, oY = 0.0f;
	float ratioX, ratioY;
	float newwidth, newheight;

	SDL_GetTextureSize(texture.get(), &w, &h);

	ratioX = width / w;
	ratioY = height / h;

	if (ratioX > ratioY)
		ratioX = ratioY;
	else
		ratioY = ratioX;

	newwidth = w * ratioX;
	newheight = h * ratioY;

	oX = (width - newwidth) / 2.0f;
	oY = (height - newheight) / 2.0f;

	rect.x = X + oX - delta;
	rect.y = Y + oY - delta;
	rect.w = newwidth + 2.0f * delta;
	rect.h = newheight + 2.0f * delta;

	SDL_SetRenderDrawColor(renderer, R(color), G(color), B(color),
			Alpha(color));
	SDL_SetTextureAlphaMod(texture.get(), 255);
	if (active)                              
		SDL_RenderFillRectF(renderer, &rect);
	SDL_RenderCopyF(renderer, texture.get(), nullptr, &rect);

	if (overlay != "") {
		float newX, newY, newW, newH;

		newX = canvas->scr_to_X(X);
		newY = canvas->scr_to_Y(Y + height/2);
		newW = canvas->scale_to_X(width);
		newH = canvas->scale_to_Y(height/2);
		canvas->draw_text(overlay, newX - canvas->scale_to_X(delta), newY - canvas->scale_to_Y(delta), newW + canvas->scale_to_X(delta), newH + canvas->scale_to_Y(delta));
	}
}

std::unique_ptr<class element> icon::create_element(void)
{
	if (class_id == "library")
		return library_element();
	return element_from_class_id(class_id);
}


std::unique_ptr<class element> icon::library_element(void)
{
	auto element = std::make_unique<model_nest>(-10, -10);
	element->load_scene_from_json(lib.logic);
	element->update_name(lib.name);
	element->set_icon(lib.icon, lib.icon_selected);
	element->set_library_origin(lib.collection, lib.name);
	return element;
}

oneiconbar::oneiconbar(SDL_Renderer *_renderer, SDL_Rect _rect, std::string _name, class basecanvas *_canvas)
{
	renderer = _renderer;
	rect = _rect;
	icons.resize(2);

	icons[0].resize(15);
	icons[1].resize(15);

	name = _name;
	canvas = _canvas;
}

void oneiconbar::resize(SDL_Rect _rect)
{
	rect = _rect;
}

float oneiconbar::cell_scale(void) const
{
	return std::max(16.0f, static_cast<float>(rect.w) / 5.5f);
}

float oneiconbar::top_offset(void) const
{
	return 0.5f * cell_scale();
}

extern float ratio_effect(float ratio);
static float delta_hover(uint64_t deltaT)
{
	float f;
	if (deltaT > 300)
		return 0;
		
	f = ratio_effect(deltaT/300.0);
	f = (1 - (0.8 + 0.2 * f)) * 15;
	return f;
	
}

void oneiconbar::draw(void)
{
	int x,y;
	SDL_RenderSetClipRect(renderer, &rect);    
	int color = 0;
	SDL_FRect box;
	const float scale = cell_scale();
	const float offset_y = top_offset();

	for (y = 0; y < 15; y++) {
		for (x = 0; x < 2; x++) {
			float delta = 0.0;
			if (hover_item_x == x && hover_item_y == y && icons[x][y]) {
				uint64_t now = SDL_GetTicks64();
				delta = delta_hover(now - hover_time) * (scale / 40.0f);
			}
			box.x = rect.x + (0.5f + 2.5f * x) * scale - delta;
			box.y = rect.y + (0.5f + 2.5f * y) * scale + offset_y - delta;
			box.h = 2.0f * scale + delta * 2.0f;
			box.w = 2.0f * scale + delta * 2.0f;
			SDL_SetRenderDrawColor(renderer, R(color), G(color), B(color),
					Alpha(color));
			SDL_RenderFillRectF(renderer, &box);
			if (icons[x][y])
				icons[x][y]->draw(renderer, rect.x + (0.6f + 2.5f * x) * scale, rect.y + (0.6f + 2.5f * y) * scale + offset_y, 1.8f * scale, 1.8f * scale, canvas, delta);


		}    
	}
}

class icon * oneiconbar::current_icon(int ScreenX, int ScreenY)
{
	float dX, dY;
	int gridX, gridY;
	const float scale = cell_scale();
	const float offset_y = top_offset();

	ScreenX -= rect.x;
	ScreenY -= rect.y;

	/* subtract the border */
	ScreenX -= 0.5f * scale;
	ScreenY -= 0.5f * scale;

	if (ScreenX < 0 || ScreenY <0) /* left of the icons */
		return nullptr;

	dX = ScreenX / (2.5f * scale);
	dY = (ScreenY - offset_y) / (2.5f * scale);

	gridX = floor(dX);
	gridY = floor(dY);

	if (gridX < 0)
		return nullptr;
	if (gridY < 0)
		return nullptr;    
	if (gridX >= (int)icons.size())
		return nullptr;
	if (gridY >= (int)icons[gridX].size())
		return nullptr;

	if (icons[gridX][gridY])
		icons[gridX][gridY]->set_active();

	return icons[gridX][gridY].get();

}

std::string oneiconbar::current_tooltip(int ScreenX, int ScreenY)
{
	float dX, dY;
	int gridX, gridY;
	const float scale = cell_scale();
	const float offset_y = top_offset();

	ScreenX -= rect.x;
	ScreenY -= rect.y;

	/* subtract the border */
	ScreenX -= 0.5f * scale;
	ScreenY -= 0.5f * scale;

	if (ScreenX < 0 || ScreenY <0) /* left of the icons */
		return "";

	dX = ScreenX / (2.5f * scale);
	dY = (ScreenY - offset_y) / (2.5f * scale);

	gridX = floor(dX);
	gridY = floor(dY);
	if (gridX < 0)
		return "";
	if (gridY < 0)
		return "";    

	if (gridX >= (int)icons.size())
		return "";
	if (gridY >= (int)icons[gridX].size())
		return "";

	if (icons[gridX][gridY])
		return icons[gridX][gridY]->get_tooltip();
	return "";
}


void iconbar::next(void)
{
	if (active_index + 1 < icons.size())
		active_index++;
	current_icons = icons[active_index].get();
}

void iconbar::previous(void)
{
	if (active_index >= 1)
		active_index--;
	current_icons = icons[active_index].get();
}


void iconbar::set(class oneiconbar *b)
{
	current_icons = b;
	for (unsigned int i = 0; i < icons.size(); i++) {
		if (icons[i].get() == b)
			active_index = i;
	}
}

void iconbar::create_menu(void)
{
	unsigned int li;

	menu.reset();
	current_icons = nullptr;
	icons.clear();
	libtabs.clear();

	auto init_icon = std::make_unique<class oneiconbar>(renderer, rect, "Logic", canvas);

	init_icon->set_element("model_zero:", "assets/zero.png", "constant zero value");
	init_icon->set_element("model_one:", "assets/one.png", "constant one value");
	init_icon->set_element("model_not:", "assets/inverter.png", "bit inverter");
	init_icon->set_element("model_buffer:", "assets/buffer.png", "buffer with Z-state output");
	init_icon->set_element("model_nand:", "assets/nandgate.png", "bitwise NAND gate");
	init_icon->set_element("model_and:", "assets/andgate.png", "bitwise AND gate");
	init_icon->set_element("model_nor:", "assets/norgate.png", "bitwise NOR gate");
	init_icon->set_element("model_or:", "assets/orgate.png", "bitwise OR gate");
	init_icon->set_element("model_xor:", "assets/xorgate.png", "bitwise XOR gate");
	init_icon->insert_blank();
	init_icon->set_element("model_toggle:", "assets/toggle_off.png", "Input element");
	init_icon->set_element("model_output:", "assets/output_on.png", "Output element");
	init_icon->set_element("model_nest:", "assets/nest/nest_icon.png", "Nested scene");
	init_icon->set_element("model_truth:", "assets/model_truth/truthtable_icon.png", "Truth table");
	init_icon->set_element("model_dflipflop:", "assets/dflipflop.png", "D flip flop");
	init_icon->set_element("model_deflipflop:", "assets/deflipflop.png", "D flip flop with enable signal");
	init_icon->set_element("model_clock:", "assets/clock_on.png", "Clock");
	init_icon->set_element("sequencer:", "assets/sequencer.png", "Sequencer");
	init_icon->set_element("model_datascope:", "assets/datascope.png", "Data scope");
	init_icon->set_element("model_label:", "assets/label.png", "Area Label");

	current_icons = init_icon.get();
	icons.push_back(std::move(init_icon));

	init_icon = std::make_unique<class oneiconbar>(renderer, rect, "Bus converters", canvas);    
	init_icon->set_element("model_4to1:", "assets/4to1.png", "Bus to 4 bits split");
	init_icon->set_element("model_1to4:", "assets/1to4.png", "4 bits to Bus concentrator");
	init_icon->set_element("model_8to1:", "assets/8to1.png", "Bus to 8 bits split");
	init_icon->set_element("model_1to8:", "assets/1to8.png", "8 bits to Bus concentrator");
	init_icon->set_element("model_8to4:", "assets/8to4.png", "bus8 to 2 bus4 split");
	init_icon->set_element("model_4to8:", "assets/4to8.png", "2 bus4 to bus8 concentrator");
	init_icon->set_element("model_16to8:", "assets/16to8.png", "bus16 to 2 bus8 split");
	init_icon->set_element("model_8to16:", "assets/8to16.png", "2 bus8 to bus16 concentrator");
	icons.push_back(std::move(init_icon));

	init_icon = std::make_unique<class oneiconbar>(renderer, rect, "Memory", canvas);
	init_icon->set_element("model_memory:", "assets/memory.png", "8 Bit Memory");
	init_icon->insert_blank();
	init_icon->set_element("model_clock:", "assets/clock_on.png", "Clock");
	init_icon->set_element("model_dflipflop:", "assets/dflipflop.png", "D flip flop");
	icons.push_back(std::move(init_icon));


	for (li = 0; li < library.size(); li++) {
		if (libtabs.find(library[li].collection) == libtabs.end()) {
			auto newtab = std::make_unique<class oneiconbar>(renderer, rect, library[li].collection, canvas);
			libtabs[library[li].collection] = newtab.get();
			icons.push_back(std::move(newtab));
		}
		class oneiconbar *libtab = libtabs[library[li].collection];
		libtab->set_element("library", &library[li]);
	}

	menu = std::make_unique<icon_contextmenu>(this);
	for (auto &ib : icons) {
		menu->add_item(ib->get_name(), ib.get(), callback_ib);
	}

}
