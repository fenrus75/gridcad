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
#include <SDL2/SDL_image.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_ttf.h>
#include <map>

extern std::map<std::string, const unsigned char *> datamap;
extern std::map<std::string, unsigned int> sizemap;

basecanvas::basecanvas(void)
{
}

int basecanvas::X_to_scr(float X)
{
	return (X - offsetX) * scaleX;
}

float basecanvas::scr_to_X(int X)
{
	return X / scaleX + offsetX;
}

int basecanvas::Y_to_scr(float Y)
{
	return (Y - offsetY) * scaleY;
}

float basecanvas::scr_to_Y(int Y)
{
	return Y / scaleY + offsetY;
}

void basecanvas::draw_box(float X1, float Y1, float X2, float Y2, int color, int alpha)
{
	int x1, y1, x2, y2;

	SDL_Rect rect;

	x1 = X_to_scr(X1);
	x2 = X_to_scr(X2);
	y1 = Y_to_scr(Y1);
	y2 = Y_to_scr(Y2);

	/* clip */
	if (x1 < 0)
		x1 = 0;
	if (x2 < 0)
		return;
	if (y1 < 0)
		y1 = 0;
	if (y2 < 0)
		return;

	/* TODO: clip boxes that are fully outside of the visible window */

	rect.x = x1;
	rect.y = y1;
	rect.w = x2 - x1;
	rect.h = y2 - y1;

	if (alpha < 0)
		alpha = Alpha(color);

	SDL_SetRenderDrawColor(renderer.get(), R(color), G(color), B(color), alpha);
	SDL_RenderFillRect(renderer.get(), &rect);
}

void basecanvas::draw_line(float X1, float Y1, float X2, float Y2, int color)
{
	/* TODO: clip boxes that are fully outside of the visible window */

	thickLineRGBA(renderer.get(), X_to_scr(X1), Y_to_scr(Y1), X_to_scr(X2),
			Y_to_scr(Y2), 0.2 * scaleX, R(color), G(color), B(color),
			Alpha(color));
	if (Alpha(color) > 250) {
		filledCircleRGBA(renderer.get(), X_to_scr(X1), Y_to_scr(Y1), 0.1 * scaleX, R(color), G(color),
				B(color), Alpha(color));
		filledCircleRGBA(renderer.get(), X_to_scr(X2), Y_to_scr(Y2), 0.1 * scaleX, R(color), G(color),
				B(color), Alpha(color));
	}
}

void basecanvas::draw_thick_line(float X1, float Y1, float X2, float Y2, int color)
{
	/* TODO: clip boxes that are fully outside of the visible window */

	thickLineRGBA(renderer.get(), X_to_scr(X1), Y_to_scr(Y1), X_to_scr(X2),
			Y_to_scr(Y2), 0.3 * scaleX, R(color), G(color), B(color),
			Alpha(color));
	if (Alpha(color) > 250) {
		filledCircleRGBA(renderer.get(), X_to_scr(X1), Y_to_scr(Y1), 0.15 * scaleX, R(color), G(color),
				B(color), Alpha(color));
		filledCircleRGBA(renderer.get(), X_to_scr(X2), Y_to_scr(Y2), 0.15 * scaleX, R(color), G(color),
				B(color), Alpha(color));
	}
}
void basecanvas::draw_shadow_Line(float X1, float Y1, float X2, float Y2, int color)
{
	int r = 0,g = 0,b = 0;
	
	if (color == COLOR_VALUE_RED)
		r = 255;
	
	/* TODO: clip boxes that are fully outside of the visible window */

	thickLineRGBA(renderer.get(), X_to_scr(X1), Y_to_scr(Y1), X_to_scr(X2),
			Y_to_scr(Y2), 0.25 * scaleX, r, g, b, 
			64);
}

void basecanvas::draw_circle(float X, float Y, float _R, int color, int color2)
{
	int x1, y1;

	x1 = X_to_scr(X);
	y1 = Y_to_scr(Y);

	/* TODO: clip boxes that are fully outside of the visible window */

	filledCircleRGBA(renderer.get(), x1, y1, _R * scaleX, R(color), G(color),
			B(color), Alpha(color));
	color = color2;
	filledCircleRGBA(renderer.get(), x1, y1, (_R / 2) * scaleX, R(color),
			G(color), B(color), Alpha(color));

}
void basecanvas::draw_circle2(float X, float Y, float _R, int color, int color2)
{
	int x1, y1;

	x1 = X_to_scr(X);
	y1 = Y_to_scr(Y);

	/* TODO: clip boxes that are fully outside of the visible window */

	filledCircleRGBA(renderer.get(), x1, y1, _R * scaleX, R(color), G(color),
			B(color), Alpha(color));
	color = color2;
	filledCircleRGBA(renderer.get(), x1, y1, (_R / 1.4) * scaleX, R(color),
			G(color), B(color), Alpha(color));

}

sdl::TexturePtr IMG_LoadTextureFromMem(SDL_Renderer *renderer, const char *filename)
{
	std::string s;

	s = filename;
	if (datamap.find(s) == datamap.end()) {
		logger::get().debug("PNGMAP miss for '{}'", filename);
		return sdl::TexturePtr(IMG_LoadTexture(renderer, filename));
	}

	SDL_RWops* ops;

	ops = SDL_RWFromConstMem(datamap[filename], sizemap[filename]);

	return sdl::TexturePtr(IMG_LoadTexture_RW(renderer, ops, 1));
}

sdl::TexturePtr basecanvas::load_image(std::string_view filename)
{
	std::string key(filename);
	return IMG_LoadTextureFromMem(renderer.get(), key.c_str());
}

void basecanvas::draw_image(SDL_Texture *image, float X, float Y, float W, float H, int alpha, bool keep_aspect) 
{
	SDL_Rect rect;
	SDL_Point size;
	float w, h;

	if (!image) {
		//		printf("Attempting to draw a null texture\n");
		return;
	}


	SDL_QueryTexture(image, nullptr, nullptr, &size.x, &size.y);

	if (keep_aspect) { 
		h = H;
		w = 1.0 * size.x / size.y * h;
		if (w > W) {
			w = W;
			h = 1.0 * size.y / size.x * w;
		}
	} else {
		w = W;
		h = H;
	}
	rect.x = X_to_scr(X) + (W-w)/2 * scaleX;
	rect.y = Y_to_scr(Y) + (H-h)/2 * scaleY;
	rect.w = w * scaleX;
	rect.h = h * scaleY;



	SDL_SetTextureAlphaMod(image, alpha);

	SDL_RenderCopy(renderer.get(), image, nullptr, &rect);
}

void basecanvas::draw_image_fragment(SDL_Texture *image, float X, float Y, float W, float H, int fromX, int fromY, int w, int h, int angle) 
{
	SDL_Rect rect, src;

	if (!image) {
		return;
	}


	rect.x = X_to_scr(X);
	rect.y = Y_to_scr(Y);
	rect.w = W * scaleX;
	rect.h = H * scaleY;

	src.x = fromX;
	src.y = fromY;
	src.w = w;
	src.h = h;

	SDL_SetTextureAlphaMod(image, 255);

	//	SDL_RenderCopy(renderer, image, &src, &rect);
	SDL_RenderCopyEx(renderer.get(), image, &src, &rect, angle, nullptr, SDL_FLIP_NONE);
}

void basecanvas::draw_image_rotated(SDL_Texture *image, float X, float Y, float W, float H, int alpha, int angle) 
{
	SDL_Rect rect;
	SDL_Point size;

	if (!image) {
		//		printf("Attempting to draw a null texture\n");
		return;
	}


	SDL_QueryTexture(image, nullptr, nullptr, &size.x, &size.y);

	/* if we rotate something that isn't square, the "top left" point afterwards is no longer at (0,0)
	   Adjust to move it back to 0,0 by half-of-the-shape-difference
	   */
	if ((angle == 90 || angle == 270)) { X -= (H-W)/2; Y -= (W-H)/2; };
	rect.x = X_to_scr(X);
	rect.y = Y_to_scr(Y);
	rect.w = W * scaleX;
	rect.h = H * scaleY;
	if (angle == 90 || angle == 270) {
		rect.w = H * scaleX;
		rect.h = W * scaleY;
	}

	SDL_SetTextureAlphaMod(image, alpha);

	SDL_RenderCopyEx(renderer.get(), image, nullptr, &rect, angle, nullptr, SDL_FLIP_NONE);
}

void basecanvas::draw_image(std::string_view filename, float X, float Y, float W, float H, int alpha, bool keep_aspect) 
{
	auto it = texture_cache.find(filename);
	if (it == texture_cache.end()) {
		sdl::TexturePtr loaded = load_image(filename);
		if (!loaded) {
			logger::get().error("Failure to load {}", filename);
		}
		it = texture_cache.emplace(std::string(filename), std::move(loaded)).first;
		if ((texture_cache.size() % 100) == 0)
			logger::get().debug("Texture cache reaches {}", texture_cache.size());
	}
	draw_image(it->second.get(), X, Y, W, H, alpha, keep_aspect);
}

void basecanvas::draw_image_rotated(std::string_view filename, float X, float Y, float W, float H, int alpha, int angle) 
{
	auto it = texture_cache.find(filename);
	if (it == texture_cache.end()) {
		sdl::TexturePtr loaded = load_image(filename);
		if (!loaded) {
			logger::get().error("Failure to load {}", filename);
		}
		it = texture_cache.emplace(std::string(filename), std::move(loaded)).first;
		if ((texture_cache.size() % 100) == 0)
			logger::get().debug("Texture cache reaches {}", texture_cache.size());
	}
	draw_image_rotated(it->second.get(), X, Y, W, H, alpha, angle);
}

void basecanvas::draw_image_fragment(std::string_view filename, float X, float Y, float W, float H, int fromX, int fromY, int w, int h, int angle)  
{
	auto it = texture_cache.find(filename);
	if (it == texture_cache.end()) {
		sdl::TexturePtr loaded = load_image(filename);
		if (!loaded) {
			logger::get().error("Failure to load {}", filename);
		}
		it = texture_cache.emplace(std::string(filename), std::move(loaded)).first;
		if ((texture_cache.size() % 100) == 0)
			logger::get().debug("Texture cache reaches {}", texture_cache.size());
	}
	draw_image_fragment(it->second.get(), X, Y, W, H, fromX, fromY, w, h, angle);
}


sdl::TexturePtr basecanvas::render_text_to_texture(const char *text)
{
	const SDL_Color white = {255, 255, 255, 255};

	if (!font)
		font.reset(TTF_OpenFont("fonts/Roboto-Medium-webfont.ttf", 28));

	if (text[0] == '*') {
		TTF_SetFontStyle(font.get(), TTF_STYLE_ITALIC);
		text++;
	} else {
		TTF_SetFontStyle(font.get(), TTF_STYLE_NORMAL);
	}
	sdl::SurfacePtr surface(TTF_RenderUTF8_Blended(font.get(), text, white));
	if (!surface) {
		logger::get().error("text_to_texture error {}", SDL_GetError());
	}
	return sdl::TexturePtr(SDL_CreateTextureFromSurface(renderer.get(), surface.get()));
}

void basecanvas::draw_text(std::string_view text, float X, float Y, float W, float H)
{
	SDL_Texture *texture;
	double w,h;
	SDL_Point size;
	
	if (text.empty())
		return;

	texture = text_to_texture(text);
	SDL_QueryTexture(texture, nullptr, nullptr, &size.x, &size.y);

	h = H;
	w = 1.0 * size.x / size.y * h;
	if (w > W) {
		w = W;
		h = 1.0 * size.y / size.x * w;
	}

	draw_image(texture, X + (W - w)/2, Y + (H - h)/2, w, h); 
}
void basecanvas::draw_text_left(std::string_view text, float X, float Y, float W, float H)
{
	SDL_Texture *texture;
	double w,h;
	SDL_Point size;
	
	if (text.empty())
		return;

	texture = text_to_texture(text);
	SDL_QueryTexture(texture, nullptr, nullptr, &size.x, &size.y);

	h = H;
	w = 1.0 * size.x / size.y * h;
	if (w > W) {
		w = W;
		h = 1.0 * size.y / size.x * w;
	}

	draw_image(texture, X, Y + (H - h), w, h); 
}

SDL_Texture *basecanvas::text_to_texture(std::string_view text)
{
	auto it = text_cache.find(text);
	if (it == text_cache.end()) {
		std::string key(text);
		sdl::TexturePtr rendered = render_text_to_texture(key.c_str());
		if (!rendered) {
			logger::get().error("Failure to render {}", text);
		}
		it = text_cache.emplace(std::move(key), std::move(rendered)).first;
		if ((text_cache.size() % 100) == 0)
			logger::get().debug("Text cache reaches {}", text_cache.size());
	}

	return it->second.get();
}

extern float ratio_effect(float ratio);
extern float animation_time;

void basecanvas::draw(void)
{
	if (in_animation) {
		uint64_t now = SDL_GetTicks64();
		
		float ratio;
		float effect;
		float effect2;
		ratio = (now - animation_start) / animation_time;
		
		if (animation_type == 1)
			ratio = ratio * 2;
		if (animation_type == 2)
			ratio = ratio * 6;
		if (ratio > 1.0)
			ratio = 1.0;
			
		effect = ratio_effect(ratio);
		effect2 = ratio * 2;
		if (animation_type == 1 || animation_type == 2) {
			effect = ratio;
			effect2 = ratio;
		}
		if (effect2 > 1.0)
			effect2 = 1.0;
		
		if (ratio >= 1.0) {
			scaleX = target_scaleX;
			scaleY = target_scaleY;
			offsetX = target_offsetX;
			offsetY = target_offsetY;
			in_animation = false;
			animation_type = 0;
			return;
		}
		
		scaleX = exp( log(from_scaleX) + effect * (log(target_scaleX) - log(from_scaleX)));
		scaleY = exp( log(from_scaleY) + effect * (log(target_scaleY) - log(from_scaleY)));
		offsetX = from_offsetX + effect2 * (target_offsetX - from_offsetX);
		offsetY = from_offsetY + effect2 * (target_offsetY - from_offsetY);
			
	}
}

bool basecanvas::handle_event(SDL_Event &event)
{
	return false;
}

void basecanvas::unhide(void)
{
	SDL_ShowWindow(window.get());
}
