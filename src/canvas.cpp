#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_ttf.h>

#include <stdio.h>

#include "gridcad.h"

#include "iconbar.h"


canvas::canvas(class scene *_scene)
{
	window =
	    SDL_CreateWindow("FOO BAR", SDL_WINDOWPOS_UNDEFINED,
			     SDL_WINDOWPOS_UNDEFINED, 1024, 768, SDL_WINDOW_RESIZABLE);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	
	main_area_rect.x = 0;
	main_area_rect.y = 0;
	main_area_rect.w = 820;
	main_area_rect.h = 768;
	ui_area_rect.x = 800;
	ui_area_rect.y = 0;
	ui_area_rect.w = 1024 - 820;
	ui_area_rect.h = 768;

	offsetX = 0;
	offsetY = 0;
	scaleX = 25;
	scaleY = 25;

	dragging = NULL;
	left_mouse_down = false;
	dragging_port = NULL;
	dragging_wire = NULL;
	icon_bar = new iconbar(renderer, ui_area_rect);
	current_scene = _scene;
}

canvas::~canvas(void)
{
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
}

void canvas::eventloop(void)
{
	SDL_Event event;
	int ret = 0;
	bool leave = false;

	while (!leave) {
		if (!ret)
			SDL_Delay(1);
		ret = SDL_PollEvent(&event);
		
		switch (event.type) {
		case SDL_QUIT:
			leave = true;
			break;
		case SDL_KEYDOWN:
			switch (event.key.keysym.sym) {
			case SDLK_UP:
				offsetY--;
				if (offsetY < -1)
					offsetY = -1;
				break;
			case SDLK_DOWN:
				offsetY++;
				if (offsetY > current_scene->sizeY + 1)
					offsetY = current_scene->sizeY;
				break;
			case SDLK_LEFT:
				offsetX--;
				if (offsetX < -1)
					offsetX = -1;
				break;
			case SDLK_RIGHT:
				offsetX++;
				if (offsetX > current_scene->sizeX + 1)
					offsetX = current_scene->sizeX;
				break;
			case SDLK_PLUS:
			case SDLK_KP_PLUS:
			case SDLK_EQUALS:
				scaleX++;
				scaleY++;
				break;
			case SDLK_MINUS:
			case SDLK_KP_MINUS:
				if (scaleX > 1)
					scaleX--;
				if (scaleY > 1)
					scaleY--;
				break;
			case SDLK_ESCAPE:
				if (floating)
					delete floating;
				floating = NULL;
				if (active_icon)
					active_icon->set_inactive();
				active_icon = NULL;
				break;
			case SDLK_BACKSPACE:
				if (hover_wire) {
					hover_wire->remove();
					delete hover_wire;
					hover_wire = NULL;
				}
				break;
			}
			break;
		case SDL_MOUSEBUTTONDOWN:
			if (!left_mouse_down
			    && event.button.button == SDL_BUTTON_LEFT) {
				dragging = NULL;
				dragging_port = NULL;
				left_mouse_down = true;
				class icon *this_icon;
				float x, y;
				
				click_start_X = event.motion.x;
				click_start_Y = event.motion.y;

				x = scr_to_X(event.motion.x);
				y = scr_to_Y(event.motion.y);

				for (auto elem:	current_scene->elements) {
					if (elem->intersect(x, y)) {
						printf("Start drag: %5.2f %5.2f \n", x, y);
						dragging = elem;
					}
				}
				if (dragging)
					dragging->start_drag(x, y);

				if (!dragging) {
					dragging_port = current_scene->is_port(x, y);
					if (dragging_port) {
						dragging_port->screenX = x;
						dragging_port->screenY = y;

						dragging_wire =
						    new wire(floorf(x),
							     floorf(y),
							     floorf(x),
							     floorf(y));
					}
				}
				
				this_icon = icon_bar->current_icon(event.motion.x, event.motion.y);
				if (active_icon && this_icon)
					active_icon->set_inactive();
				if (active_icon != this_icon && this_icon) {
					active_icon = this_icon;
					delete floating;
					floating = NULL;
				}				
				else
					if (this_icon)
						active_icon = NULL;
					
				if (active_icon) {
					if (!floating)
						floating = active_icon->create_element();
				} else {
					delete floating;
					floating = NULL;
				}
				
				
				if (event.motion.x < main_area_rect.w && floating) {
					printf("Adding element\n");
					current_scene->add_element(floating);
					floating->stop_drag(this);
					if (active_icon)
						floating = active_icon->create_element();
					else
						floating = NULL;
				}


			}
			if ( event.button.button == SDL_BUTTON_RIGHT) {
				printf("Right button\n");
			}
			break;
		case SDL_MOUSEBUTTONUP:
			if (event.button.button == SDL_BUTTON_LEFT) {
				left_mouse_down = false;
				float x, y;
				x = scr_to_X(event.motion.x);
				y = scr_to_Y(event.motion.y);
				if (dragging) {
					printf("Up\n");
					if (!dragging->has_moved())  {
						printf("Click\n");
						dragging->mouse_select();
					}
					dragging->stop_drag(this);
				}
				if (dragging_port && !current_scene->is_port(x, y)) {
					class element *_element;
					_element = new connector(x, y);
				        current_scene->add_element(_element);
				}
				if (dragging_port && current_scene->is_port(x, y)) {
					class port *port2 = current_scene->is_port(x, y);
					
					if (dragging_port != port2) {
						dragging_port->add_wire(dragging_wire);
						port2->add_wire(dragging_wire);
						dragging_wire->route(current_scene);
						dragging_wire = NULL;
						printf("Connection made\n");
					}
				} 
				dragging = NULL;
				dragging_port = NULL;
				left_mouse_down = false;
				if (dragging_wire)
					delete dragging_wire;
				dragging_wire = NULL;
			}
			break;
		case SDL_MOUSEMOTION:
			int x,y;
			mouseX = scr_to_X(event.motion.x);
			mouseY = scr_to_Y(event.motion.y);
			x = scr_to_X(event.motion.x);
			y = scr_to_Y(event.motion.y);
			
			if (hover_wire)
				hover_wire->deselect();
			hover_wire = current_scene->is_wire(x, y);
			if (hover_wire)
				hover_wire->select();

			if (dragging) {
				dragging->update_drag(this, current_scene,
						      scr_to_X(event.motion.x),
						      scr_to_Y(event.motion.y));
			}
			if (floating) {
				floating->update_drag(this, current_scene,
						      scr_to_X(event.motion.x),
						      scr_to_Y(event.motion.y));
			}
			if (dragging_wire) {
				bool banned = false;
				for (auto elem:	current_scene->elements) {
					if (elem->intersect(x, y))
						banned = true;
				}
				if (!banned)
					dragging_wire->move_target(mouseX, mouseY);
			}
			break;
			
		case SDL_MOUSEWHEEL:
			if (event.wheel.y > 0) {
				float cX, cY;
				int count = 0;
				
				cX = X_to_scr(mouseX);
				cY = Y_to_scr(mouseY);
				scaleX++;
				scaleY++;
				
				/* need to keep the point under the cursor the same */
				while (scr_to_X(cX) < mouseX && count++ < 1000)
					offsetX += 0.02;
				while (scr_to_Y(cY) < mouseY && count++ < 1000)
					offsetY += 0.02;
				
			}
			if (event.wheel.y < 0) {
				float cX, cY;
				int count = 0;
				
				cX = X_to_scr(mouseX);
				cY = Y_to_scr(mouseY);
				if (scaleX > 1)
					scaleX--;
				if (scaleY > 1)
					scaleY--;
				/* need to keep the point under the cursor the same */
				while (scr_to_X(cX) > mouseX && count++ < 1000)  
					offsetX -= 0.2;
				while (scr_to_Y(cY) > mouseY && count++ < 1000)
					offsetY -= 0.2;
			}
			
			break;

		case SDL_POLLSENTINEL:
			break;
		case SDL_WINDOWEVENT:
			if (event.window.event == SDL_WINDOWEVENT_RESIZED) {			
				main_area_rect.w = event.window.data1 - 220;;
				main_area_rect.h = event.window.data2;
				ui_area_rect.x = main_area_rect.w;
				ui_area_rect.y = 0;
				ui_area_rect.w = event.window.data1 - main_area_rect.w;
				ui_area_rect.h = event.window.data2;
				icon_bar->resize(ui_area_rect);
			}
			break;
		}		
		draw();
	}
}

void canvas::draw(void)
{
	/* first, draw the lighter gray background */
	SDL_SetRenderDrawColor(renderer, R(COLOR_BACKGROUND_GRID),
			       G(COLOR_BACKGROUND_GRID),
			       B(COLOR_BACKGROUND_GRID),
			       Alpha(COLOR_BACKGROUND_GRID));
	SDL_RenderClear(renderer);
	
	icon_bar->draw();
	SDL_RenderSetClipRect(renderer, &main_area_rect);

	/* then fill in the darger gray grid to keep lines */
	float X = 0, Y = 0;
	while (Y < current_scene->sizeY) {
		X = 0;
		while (X < current_scene->sizeX) {
			drawBox(X + 0.2, Y + 0.2, X + 9.8, Y + 9.8,
				COLOR_BACKGROUND_MAIN);
			X = X + 10;
		}
		Y = Y + 10;
	}

	/* draw the elements */
	for (auto const elem: current_scene->elements) {
		if (elem != dragging)
			elem->draw(this, DRAW_NORMAL);
		else
			elem->draw(this, DRAW_ORIGIN);
	}

	if (dragging) {
		dragging->draw(this, DRAW_GHOST);
		dragging->draw(this, DRAW_DND);
	}
	if (floating) {
		floating->draw(this, DRAW_GHOST);
		floating->draw(this, DRAW_DND);
	}

	if (dragging_wire) {
		class port *port2;
		int color = COLOR_WIRE_INVALID;

		port2 = current_scene->is_port(mouseX, mouseY);
		if (port2)
			color = COLOR_WIRE_SOLID;

		dragging_wire->draw(this, color);
	}

	SDL_RenderPresent(renderer);
}

int canvas::X_to_scr(float X)
{
	return (X - offsetX) * scaleX;
}

float canvas::scr_to_X(int X)
{
	return X / scaleX + offsetX;
}

int canvas::Y_to_scr(float Y)
{
	return (Y - offsetY) * scaleY;
}

float canvas::scr_to_Y(int Y)
{
	return Y / scaleY + offsetY;
}

void canvas::drawBox(float X1, float Y1, float X2, float Y2, int color)
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

	SDL_SetRenderDrawColor(renderer, R(color), G(color), B(color),
			       Alpha(color));
	SDL_RenderFillRect(renderer, &rect);
}

void canvas::drawLine(float X1, float Y1, float X2, float Y2, int color)
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

void canvas::drawCircle(float X, float Y, float _R, int color, int color2)
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

SDL_Texture * canvas::load_image(const char *filename)
{
	return IMG_LoadTexture(renderer, filename);
}

void canvas::draw_image(SDL_Texture *image, float X, float Y, float W, float H, int alpha) 
{
	SDL_Rect rect;
	rect.x = X_to_scr(X);
	rect.y = Y_to_scr(Y);
	rect.w = W * scaleX;
	rect.h = H * scaleY;
	
	SDL_SetTextureAlphaMod(image, alpha);
	
	SDL_RenderCopy(renderer, image, NULL, &rect);
}


SDL_Texture *canvas::text_to_texture(const char *text)
{
	const SDL_Color white = {255, 255, 255, 255};
	SDL_Texture *tx;
	SDL_Surface *surface;
	TTF_Font *font;
		
	font = TTF_OpenFont("fonts/Roboto-Medium-webfont.ttf", 14);
	surface = TTF_RenderUTF8_Blended(font, text, white);
	tx = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_FreeSurface(surface);
	TTF_CloseFont(font);
	return tx;
}
