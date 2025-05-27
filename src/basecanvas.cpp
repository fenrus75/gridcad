#include "gridcad.h"
#include <SDL2/SDL_image.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_ttf.h>

basecanvas::basecanvas(void)
{
}

basecanvas::~basecanvas(void)
{
	printf("basecanvas destructor\n");
//	if (font)
//		TTF_CloseFont(font);
//	font = NULL;
	for (auto const& [key, val] : text_cache)
	{
		SDL_DestroyTexture(val);
	}
	text_cache.clear();
	for (auto const& [key, val] : texture_cache)
	{
		SDL_DestroyTexture(val);
	}
	texture_cache.clear();
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

	SDL_SetRenderDrawColor(renderer, R(color), G(color), B(color), alpha);
	SDL_RenderFillRect(renderer, &rect);
}

void basecanvas::draw_line(float X1, float Y1, float X2, float Y2, int color)
{
	/* TODO: clip boxes that are fully outside of the visible window */

	thickLineRGBA(renderer, X_to_scr(X1), Y_to_scr(Y1), X_to_scr(X2),
		      Y_to_scr(Y2), 0.2 * scaleX, R(color), G(color), B(color),
		      Alpha(color));
	if (Alpha(color) > 250) {
		filledCircleRGBA(renderer, X_to_scr(X1), Y_to_scr(Y1), 0.1 * scaleX, R(color), G(color),
			 B(color), Alpha(color));
		filledCircleRGBA(renderer, X_to_scr(X2), Y_to_scr(Y2), 0.1 * scaleX, R(color), G(color),
			 B(color), Alpha(color));
	}
}
void basecanvas::draw_shadow_Line(float X1, float Y1, float X2, float Y2, int color)
{
	/* TODO: clip boxes that are fully outside of the visible window */

	thickLineRGBA(renderer, X_to_scr(X1), Y_to_scr(Y1), X_to_scr(X2),
		      Y_to_scr(Y2), 0.25 * scaleX, 0, 0, 0, 
		      64);
}

void basecanvas::draw_circle(float X, float Y, float _R, int color, int color2)
{
	int x1, y1;

	x1 = X_to_scr(X);
	y1 = Y_to_scr(Y);

	/* TODO: clip boxes that are fully outside of the visible window */

	filledCircleRGBA(renderer, x1, y1, _R * scaleX, R(color), G(color),
			 B(color), Alpha(color));
	color = color2;
	filledCircleRGBA(renderer, x1, y1, (_R / 2) * scaleX, R(color),
			 G(color), B(color), Alpha(color));

}
void basecanvas::draw_circle2(float X, float Y, float _R, int color, int color2)
{
	int x1, y1;

	x1 = X_to_scr(X);
	y1 = Y_to_scr(Y);

	/* TODO: clip boxes that are fully outside of the visible window */

	filledCircleRGBA(renderer, x1, y1, _R * scaleX, R(color), G(color),
			 B(color), Alpha(color));
	color = color2;
	filledCircleRGBA(renderer, x1, y1, (_R / 1.4) * scaleX, R(color),
			 G(color), B(color), Alpha(color));

}

SDL_Texture * basecanvas::load_image(const char *filename)
{
	return IMG_LoadTexture(renderer, filename);
}

SDL_Texture * basecanvas::load_image(std::string filename)
{
	return load_image(filename.c_str());
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

	
	SDL_QueryTexture(image, NULL, NULL, &size.x, &size.y);

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
	
	SDL_RenderCopy(renderer, image, NULL, &rect);
}

void basecanvas::draw_image_rotated(SDL_Texture *image, float X, float Y, float W, float H, int alpha, int angle) 
{
	SDL_Rect rect;
	SDL_Point size;
	
	if (!image) {
//		printf("Attempting to draw a null texture\n");
		return;
	}

	
	SDL_QueryTexture(image, NULL, NULL, &size.x, &size.y);

	if ((angle == 90 || angle == 270) && W != H) { X -= 0.5; Y += 0.5; };
	rect.x = X_to_scr(X);
	rect.y = Y_to_scr(Y);
	rect.w = W * scaleX;
	rect.h = H * scaleY;
	if (angle == 90 || angle == 270) {
		rect.w = H * scaleX;
		rect.h = W * scaleY;
	}

	SDL_SetTextureAlphaMod(image, alpha);
	
	SDL_RenderCopyEx(renderer, image, NULL, &rect, angle, NULL, SDL_FLIP_NONE);
}

void basecanvas::draw_image(std::string filename, float X, float Y, float W, float H, int alpha, bool keep_aspect) 
{
	SDL_Texture *image;
	
	if (texture_cache.find(filename) != texture_cache.end()) {
		image = texture_cache[filename];
	} else {
		image = load_image(filename);
		if (!image) {
			printf("Failure to load %s\n", filename.c_str());
		}
		texture_cache[filename] = image;
	}
	draw_image(image, X,Y,W,H,alpha,keep_aspect);
}

void basecanvas::draw_image_rotated(std::string filename, float X, float Y, float W, float H, int alpha, int angle) 
{
	SDL_Texture *image;
	
	if (texture_cache.find(filename) != texture_cache.end()) {
		image = texture_cache[filename];
	} else {
		image = load_image(filename);
		if (!image) {
			printf("Failure to load %s\n", filename.c_str());
		}
		texture_cache[filename] = image;
	}
	draw_image_rotated(image, X,Y,W,H,alpha, angle);
}


SDL_Texture *basecanvas::text_to_texture(const char *text)
{
	const SDL_Color white = {255, 255, 255, 255};
	SDL_Texture *tx;
	SDL_Surface *surface;
		
	if (!font)
		font = TTF_OpenFont("fonts/Roboto-Medium-webfont.ttf", 28);
	surface = TTF_RenderUTF8_Blended(font, text, white);
	tx = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_FreeSurface(surface);
	return tx;
}

void basecanvas::draw_text(std::string text, float X, float Y, float W, float H)
{
	SDL_Texture *texture;
	double w,h;
	SDL_Point size;
	
	texture = text_to_texture(text);
	SDL_QueryTexture(texture, NULL, NULL, &size.x, &size.y);
		
	h = H;
	w = 1.0 * size.x / size.y * h;
	if (w > W) {
		w = W;
		h = 1.0 * size.y / size.x * w;
	}
		
	draw_image(texture, X + (W - w)/2, Y + (H - h)/2, w, h); 
//	SDL_DestroyTexture(texture);
}

SDL_Texture *basecanvas::text_to_texture(std::string text)
{
	SDL_Texture *image;
	if (text_cache.find(text) != text_cache.end()) {
		image = text_cache[text];
	} else {
		image = text_to_texture(text.c_str());;
		if (!image) {
			printf("Failure to render  %s\n", text.c_str());
		}
		text_cache[text] = image;
	}
	
	return image;
}

void basecanvas::draw(void)
{
}

bool basecanvas::handle_event(SDL_Event &event)
{
	return false;
}

void basecanvas::unhide(void)
{
	SDL_ShowWindow(window);
}
