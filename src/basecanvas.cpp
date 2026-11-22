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
#include <unordered_map>

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
	flush_line_batch();
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

/*
 * Draw a thick line segment as a single 2-triangle quad via SDL_RenderGeometry,
 * culling segments whose bounding box lies outside the current render target.
 * Avoids SDL2_gfx thickLineRGBA(), which decomposes lines into per-scanline
 * hline() calls and explodes SDL's command buffer on large schematics.
 */
void basecanvas::flush_line_batch(void)
{
	if (line_verts.empty())
		return;
	if (renderer) {
		SDL_RenderGeometry(renderer.get(), nullptr,
		                   line_verts.data(), static_cast<int>(line_verts.size()),
		                   line_indices.data(), static_cast<int>(line_indices.size()));
	}
	line_verts.clear();
	line_indices.clear();
}

bool basecanvas::point_visible(float x, float y, float radius) const
{
	if (cached_out_w <= 0 || cached_out_h <= 0)
		return true;
	return x + radius >= 0.0f && x - radius <= static_cast<float>(cached_out_w) &&
	       y + radius >= 0.0f && y - radius <= static_cast<float>(cached_out_h);
}

void basecanvas::emit_thick_quad(float x1, float y1, float x2, float y2,
                                 float thickness, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	if (cached_out_w > 0 && cached_out_h > 0) {
		const float pad = thickness + 2.0f;
		const float min_x = std::min(x1, x2) - pad;
		const float max_x = std::max(x1, x2) + pad;
		const float min_y = std::min(y1, y2) - pad;
		const float max_y = std::max(y1, y2) + pad;
		if (max_x < 0.0f || min_x > static_cast<float>(cached_out_w) ||
		    max_y < 0.0f || min_y > static_cast<float>(cached_out_h))
			return;
	}

	if (line_verts.size() >= 32000)
		flush_line_batch();

	const float half = std::max(0.5f, thickness * 0.5f);
	const float dx = x2 - x1;
	const float dy = y2 - y1;
	const float len = std::hypot(dx, dy);

	float nx = 0.0f, ny = half;
	if (len > 1e-4f) {
		nx = (-dy / len) * half;
		ny = (dx / len) * half;
	}

	const int base = static_cast<int>(line_verts.size());
	const SDL_Color col = {r, g, b, a};
	line_verts.push_back({{x1 + nx, y1 + ny}, col, {0.0f, 0.0f}});
	line_verts.push_back({{x1 - nx, y1 - ny}, col, {0.0f, 0.0f}});
	line_verts.push_back({{x2 - nx, y2 - ny}, col, {0.0f, 0.0f}});
	line_verts.push_back({{x2 + nx, y2 + ny}, col, {0.0f, 0.0f}});
	line_indices.push_back(base + 0);
	line_indices.push_back(base + 1);
	line_indices.push_back(base + 2);
	line_indices.push_back(base + 0);
	line_indices.push_back(base + 2);
	line_indices.push_back(base + 3);
}

void basecanvas::emit_round_cap(float cx, float cy, float radius, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	if (radius < 1.0f || !point_visible(cx, cy, radius))
		return;
	if (line_verts.size() >= 32000)
		flush_line_batch();

	/* 6-sided hexagon fan in the same SDL_RenderGeometry batch (replaces per-scanline filledCircleRGBA). */
	static constexpr float kCos[6] = {1.0f, 0.5f, -0.5f, -1.0f, -0.5f, 0.5f};
	static constexpr float kSin[6] = {0.0f, 0.8660254f, 0.8660254f, 0.0f, -0.8660254f, -0.8660254f};

	const int center = static_cast<int>(line_verts.size());
	const SDL_Color col = {r, g, b, a};
	line_verts.push_back({{cx, cy}, col, {0.0f, 0.0f}});
	for (int i = 0; i < 6; ++i)
		line_verts.push_back({{cx + kCos[i] * radius, cy + kSin[i] * radius}, col, {0.0f, 0.0f}});
	for (int i = 0; i < 6; ++i) {
		line_indices.push_back(center);
		line_indices.push_back(center + 1 + i);
		line_indices.push_back(center + 1 + ((i + 1) % 6));
	}
}

void basecanvas::draw_line(float X1, float Y1, float X2, float Y2, int color)
{
	const float sx1 = static_cast<float>(X_to_scr(X1));
	const float sy1 = static_cast<float>(Y_to_scr(Y1));
	const float sx2 = static_cast<float>(X_to_scr(X2));
	const float sy2 = static_cast<float>(Y_to_scr(Y2));
	const Uint8 r = R(color), g = G(color), b = B(color), a = Alpha(color);

	emit_thick_quad(sx1, sy1, sx2, sy2, 0.2f * scaleX, r, g, b, a);
	if (a > 250) {
		const float cap_r = 0.1f * scaleX;
		emit_round_cap(sx1, sy1, cap_r, r, g, b, a);
		emit_round_cap(sx2, sy2, cap_r, r, g, b, a);
	}
}

void basecanvas::draw_thick_line(float X1, float Y1, float X2, float Y2, int color)
{
	const float sx1 = static_cast<float>(X_to_scr(X1));
	const float sy1 = static_cast<float>(Y_to_scr(Y1));
	const float sx2 = static_cast<float>(X_to_scr(X2));
	const float sy2 = static_cast<float>(Y_to_scr(Y2));
	const Uint8 r = R(color), g = G(color), b = B(color), a = Alpha(color);

	emit_thick_quad(sx1, sy1, sx2, sy2, 0.3f * scaleX, r, g, b, a);
	if (a > 250) {
		const float cap_r = 0.15f * scaleX;
		emit_round_cap(sx1, sy1, cap_r, r, g, b, a);
		emit_round_cap(sx2, sy2, cap_r, r, g, b, a);
	}
}

void basecanvas::draw_shadow_Line(float X1, float Y1, float X2, float Y2, int color)
{
	Uint8 r = 0, g = 0, b = 0;

	if (color == COLOR_VALUE_RED)
		r = 255;

	emit_thick_quad(static_cast<float>(X_to_scr(X1)), static_cast<float>(Y_to_scr(Y1)),
	                static_cast<float>(X_to_scr(X2)), static_cast<float>(Y_to_scr(Y2)),
	                0.25f * scaleX, r, g, b, 64);
}

void basecanvas::draw_circle(float X, float Y, float _R, int color, int color2)
{
	flush_line_batch();
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
	flush_line_batch();
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

static sdl::SurfacePtr downscale_surface_if_large(sdl::SurfacePtr src, int max_dim)
{
	if (!src || (src->w <= max_dim && src->h <= max_dim))
		return src;

	/* Use an exact integer divisor + premultiplied-alpha box filter to avoid fractional-scale fuzzing. */
	const int div = (std::max(src->w, src->h) + max_dim - 1) / max_dim;
	if (div <= 1)
		return src;

	const int dst_w = std::max(1, src->w / div);
	const int dst_h = std::max(1, src->h / div);

	/* RGBA32 guarantees byte order [R, G, B, A] regardless of host endianness. */
	sdl::SurfacePtr rgba(SDL_ConvertSurfaceFormat(src.get(), SDL_PIXELFORMAT_RGBA32, 0));
	if (!rgba)
		return src;
	sdl::SurfacePtr dst(SDL_CreateRGBSurfaceWithFormat(0, dst_w, dst_h, 32, SDL_PIXELFORMAT_RGBA32));
	if (!dst)
		return src;

	if (SDL_MUSTLOCK(rgba.get()))
		SDL_LockSurface(rgba.get());
	if (SDL_MUSTLOCK(dst.get()))
		SDL_LockSurface(dst.get());

	const auto *src_base = static_cast<const uint8_t *>(rgba->pixels);
	auto *dst_base = static_cast<uint8_t *>(dst->pixels);

	for (int dy = 0; dy < dst_h; ++dy) {
		const int y0 = dy * div;
		const int y1 = std::min(rgba->h, y0 + div);
		uint8_t *dst_row = dst_base + dy * dst->pitch;
		for (int dx = 0; dx < dst_w; ++dx) {
			const int x0 = dx * div;
			const int x1 = std::min(rgba->w, x0 + div);
			uint64_t sum_r = 0, sum_g = 0, sum_b = 0, sum_a = 0, count = 0;
			for (int sy = y0; sy < y1; ++sy) {
				const uint8_t *src_row = src_base + sy * rgba->pitch;
				for (int sx = x0; sx < x1; ++sx) {
					const uint8_t *px = src_row + sx * 4;
					const uint64_t a = px[3];
					sum_r += static_cast<uint64_t>(px[0]) * a;
					sum_g += static_cast<uint64_t>(px[1]) * a;
					sum_b += static_cast<uint64_t>(px[2]) * a;
					sum_a += a;
					++count;
				}
			}
			uint8_t *out = dst_row + dx * 4;
			if (count > 0 && sum_a > 0) {
				out[0] = static_cast<uint8_t>(std::min<uint64_t>(255, (sum_r + sum_a / 2) / sum_a));
				out[1] = static_cast<uint8_t>(std::min<uint64_t>(255, (sum_g + sum_a / 2) / sum_a));
				out[2] = static_cast<uint8_t>(std::min<uint64_t>(255, (sum_b + sum_a / 2) / sum_a));
				out[3] = static_cast<uint8_t>(std::min<uint64_t>(255, (sum_a + count / 2) / count));
			} else {
				out[0] = out[1] = out[2] = out[3] = 0;
			}
		}
	}

	if (SDL_MUSTLOCK(dst.get()))
		SDL_UnlockSurface(dst.get());
	if (SDL_MUSTLOCK(rgba.get()))
		SDL_UnlockSurface(rgba.get());

	return dst;
}

sdl::TexturePtr IMG_LoadTextureFromMem(SDL_Renderer *renderer, const char *filename)
{
	if (!renderer || !filename || filename[0] == '\0')
		return nullptr;

	/* Cache decoded (and downscaled) PNG surfaces once globally so opening new windows
	 * never re-runs libpng/zlib and doesn't allocate ~300 MB of 1200x1200 icons per window. */
	static std::unordered_map<std::string, sdl::SurfacePtr> surface_cache;

	std::string key(filename);
	auto it = surface_cache.find(key);
	if (it == surface_cache.end()) {
		sdl::SurfacePtr surf;
		auto dm = datamap.find(key);
		if (dm == datamap.end()) {
			logger::get().debug("PNGMAP miss for '{}'", filename);
			surf.reset(IMG_Load(filename));
		} else {
			SDL_RWops *ops = SDL_RWFromConstMem(dm->second, sizemap[key]);
			if (ops)
				surf.reset(IMG_Load_RW(ops, 1));
		}
		/* Keep clock_wave.png native since draw_image_fragment() indexes it by pixel coords. */
		if (key != "assets/clock_wave.png")
			surf = downscale_surface_if_large(std::move(surf), 256);
		it = surface_cache.emplace(std::move(key), std::move(surf)).first;
	}

	if (!it->second)
		return nullptr;

	return sdl::TexturePtr(SDL_CreateTextureFromSurface(renderer, it->second.get()));
}

sdl::TexturePtr basecanvas::load_image(std::string_view filename)
{
	std::string key(filename);
	return IMG_LoadTextureFromMem(renderer.get(), key.c_str());
}

void basecanvas::draw_image(SDL_Texture *image, float X, float Y, float W, float H, int alpha, bool keep_aspect) 
{
	flush_line_batch();
	SDL_Rect rect;
	float w, h;

	if (!image) {
		return;
	}

	if (keep_aspect) {
		SDL_Point size = {1, 1};
		SDL_QueryTexture(image, nullptr, nullptr, &size.x, &size.y);
		h = H;
		w = 1.0f * size.x / std::max(1, size.y) * h;
		if (w > W) {
			w = W;
			h = 1.0f * size.y / std::max(1, size.x) * w;
		}
	} else {
		w = W;
		h = H;
	}
	rect.x = X_to_scr(X) + (W-w)/2 * scaleX;
	rect.y = Y_to_scr(Y) + (H-h)/2 * scaleY;
	rect.w = w * scaleX;
	rect.h = h * scaleY;

	if (cached_out_w > 0 && cached_out_h > 0 &&
	    (rect.x + rect.w < 0 || rect.x > cached_out_w ||
	     rect.y + rect.h < 0 || rect.y > cached_out_h))
		return;

	SDL_SetTextureAlphaMod(image, alpha);

	SDL_RenderCopy(renderer.get(), image, nullptr, &rect);
}

void basecanvas::draw_image_fragment(SDL_Texture *image, float X, float Y, float W, float H, int fromX, int fromY, int w, int h, int angle) 
{
	flush_line_batch();
	SDL_Rect rect, src;

	if (!image) {
		return;
	}

	rect.x = X_to_scr(X);
	rect.y = Y_to_scr(Y);
	rect.w = W * scaleX;
	rect.h = H * scaleY;

	if (cached_out_w > 0 && cached_out_h > 0 &&
	    (rect.x + rect.w < 0 || rect.x > cached_out_w ||
	     rect.y + rect.h < 0 || rect.y > cached_out_h))
		return;

	src.x = fromX;
	src.y = fromY;
	src.w = w;
	src.h = h;

	SDL_SetTextureAlphaMod(image, 255);

	SDL_RenderCopyEx(renderer.get(), image, &src, &rect, angle, nullptr, SDL_FLIP_NONE);
}

void basecanvas::draw_image_rotated(SDL_Texture *image, float X, float Y, float W, float H, int alpha, int angle) 
{
	flush_line_batch();
	SDL_Rect rect;

	if (!image) {
		return;
	}

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

	if (cached_out_w > 0 && cached_out_h > 0 &&
	    (rect.x + rect.w < 0 || rect.x > cached_out_w ||
	     rect.y + rect.h < 0 || rect.y > cached_out_h))
		return;

	SDL_SetTextureAlphaMod(image, alpha);

	SDL_RenderCopyEx(renderer.get(), image, nullptr, &rect, angle, nullptr, SDL_FLIP_NONE);
}

const basecanvas::cached_texture &basecanvas::get_cached_texture(std::string_view filename)
{
	auto it = texture_cache.find(filename);
	if (it == texture_cache.end()) {
		cached_texture entry;
		entry.tex = load_image(filename);
		if (entry.tex) {
			SDL_QueryTexture(entry.tex.get(), nullptr, nullptr, &entry.w, &entry.h);
		} else {
			logger::get().error("Failure to load {}", filename);
		}
		it = texture_cache.emplace(std::string(filename), std::move(entry)).first;
		if ((texture_cache.size() % 100) == 0)
			logger::get().debug("Texture cache reaches {}", texture_cache.size());
	}
	return it->second;
}

void basecanvas::draw_image(std::string_view filename, float X, float Y, float W, float H, int alpha, bool keep_aspect) 
{
	const auto &ct = get_cached_texture(filename);
	if (!ct.tex)
		return;
	if (keep_aspect && ct.w > 0 && ct.h > 0) {
		float h = H;
		float w = 1.0f * ct.w / ct.h * h;
		if (w > W) {
			w = W;
			h = 1.0f * ct.h / ct.w * w;
		}
		draw_image(ct.tex.get(), X + (W - w) / 2.0f, Y + (H - h) / 2.0f, w, h, alpha, false);
		return;
	}
	draw_image(ct.tex.get(), X, Y, W, H, alpha, false);
}

void basecanvas::draw_image_rotated(std::string_view filename, float X, float Y, float W, float H, int alpha, int angle) 
{
	const auto &ct = get_cached_texture(filename);
	draw_image_rotated(ct.tex.get(), X, Y, W, H, alpha, angle);
}

void basecanvas::draw_image_fragment(std::string_view filename, float X, float Y, float W, float H, int fromX, int fromY, int w, int h, int angle)  
{
	const auto &ct = get_cached_texture(filename);
	draw_image_fragment(ct.tex.get(), X, Y, W, H, fromX, fromY, w, h, angle);
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

const basecanvas::cached_texture &basecanvas::get_cached_text(std::string_view text)
{
	auto it = text_cache.find(text);
	if (it == text_cache.end()) {
		std::string key(text);
		cached_texture entry;
		entry.tex = render_text_to_texture(key.c_str());
		if (entry.tex) {
			SDL_QueryTexture(entry.tex.get(), nullptr, nullptr, &entry.w, &entry.h);
		} else {
			logger::get().error("Failure to render {}", text);
		}
		it = text_cache.emplace(std::move(key), std::move(entry)).first;
		if ((text_cache.size() % 100) == 0)
			logger::get().debug("Text cache reaches {}", text_cache.size());
	}
	return it->second;
}

void basecanvas::draw_text(std::string_view text, float X, float Y, float W, float H)
{
	if (text.empty())
		return;

	const auto &ct = get_cached_text(text);
	if (!ct.tex || ct.w <= 0 || ct.h <= 0)
		return;

	double h = H;
	double w = 1.0 * ct.w / ct.h * h;
	if (w > W) {
		w = W;
		h = 1.0 * ct.h / ct.w * w;
	}

	draw_image(ct.tex.get(), X + (W - w)/2, Y + (H - h)/2, w, h); 
}
void basecanvas::draw_text_left(std::string_view text, float X, float Y, float W, float H)
{
	if (text.empty())
		return;

	const auto &ct = get_cached_text(text);
	if (!ct.tex || ct.w <= 0 || ct.h <= 0)
		return;

	double h = H;
	double w = 1.0 * ct.w / ct.h * h;
	if (w > W) {
		w = W;
		h = 1.0 * ct.h / ct.w * w;
	}

	draw_image(ct.tex.get(), X, Y + (H - h), w, h); 
}

SDL_Texture *basecanvas::text_to_texture(std::string_view text)
{
	return get_cached_text(text).tex.get();
}

extern float ratio_effect(float ratio);
extern float animation_time;

void basecanvas::draw(void)
{
	++frame_id;
	line_verts.clear();
	line_indices.clear();
	if (renderer)
		SDL_GetRendererOutputSize(renderer.get(), &cached_out_w, &cached_out_h);
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
	/* Only restore if minimized — calling SDL_RestoreWindow() on a maximized window un-maximizes it. */
	if (SDL_GetWindowFlags(window.get()) & SDL_WINDOW_MINIMIZED)
		SDL_RestoreWindow(window.get());
	SDL_ShowWindow(window.get());
	SDL_RaiseWindow(window.get());
}
