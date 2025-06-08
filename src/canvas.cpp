/* gridcad -- a tool to do digital circuit design with a GUI
 *
 * (C) Copyright 2025  -- Arjan van de Ven
 *
 * Licensed under the terms of the
 * GNU General Public LIcense version 3.0
 *
 * Authors:
 *    Arjan van de Ven <arjanvandeven@gmail.com>
 */

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_ttf.h>

#include <stdio.h>



#include "gridcad.h"
#include "connector.h"
#include "wire.h"
#include "port.h"
#include "iconbar.h"
#include "contextmenu.h"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

extern void callback_fit_to_screen(class scene *scene);

static std::string clipboard;

canvas::canvas(class scene *_scene)
{
	std::string s;
	
	s = "GridCad " + _scene->get_full_name();
	window =
		SDL_CreateWindow(s.c_str(), SDL_WINDOWPOS_UNDEFINED,
				SDL_WINDOWPOS_UNDEFINED, 1024, 768, SDL_WINDOW_RESIZABLE);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
//	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

	windowID = SDL_GetWindowID(window);

	main_area_rect.x = 0;
	main_area_rect.y = 0;
	main_area_rect.w = 820;
	main_area_rect.h = 768;
	ui_area_rect.x = 800;
	ui_area_rect.y = 0;
	ui_area_rect.w = 1024 - 820;
	ui_area_rect.h = 768;

	scaleX = 25;
	scaleY = 25;

	dragging = NULL;
	left_mouse_down = false;
	dragging_port = NULL;
	dragging_wire = NULL;
	icon_bar = new iconbar(renderer, ui_area_rect);
	current_scene = _scene;
	callback_fit_to_screen(current_scene);
}

canvas::~canvas(void)
{
	printf("Canvas destructor\n");
	unregister_canvas(this);
	SDL_DestroyRenderer(renderer);
	renderer = NULL;
	SDL_DestroyWindow(window);
	delete icon_bar;  icon_bar = NULL;
	delete current_scene;
}

bool canvas::handle_event_drawingarea(SDL_Event &event)
{
	bool freshsplit = false;
	if (!shift_down)
		current_scene->deselect_all();
	if (!left_mouse_down
			&& event.button.button == SDL_BUTTON_LEFT) {
			
		class wire *wr;
		bool is_port = false;
//		bool is_element = false;
		bool is_wire = false;
		
		/* priority order port > element > wire */
		
		if (dragging)
			dragging->deselect();
		dragging = NULL;
		dragging_port = NULL;
		left_mouse_down = true;
		float x, y;

		click_start_X = event.motion.x;
		click_start_Y = event.motion.y;

		x = scr_to_X(event.motion.x);
		y = scr_to_Y(event.motion.y);
		
		is_port = current_scene->is_port(x,y);
		
		for (auto elem:	current_scene->elements) {
			if (elem->intersect(x, y) && !elem->is_port(x,y)) {
				printf("Start drag: %5.2f %5.2f \n", x, y);
				dragging = elem;
			}
		}


		wr = current_scene->is_wire(x, y);
		if (wr)
			is_wire = true;
		if (is_wire)
			printf("WR  %i\n", dragging != NULL);


		if (!dragging && !is_wire && floating.size() == 0 && !is_port) {
			in_area_select = true;
			area_select_X1 = x;
			area_select_Y1 = y;	
			return false;
		}

		if (!dragging && wr && !is_port) {
			class wire *wr2;
			class element *_element;
			class port *port;
			take_undo_snapshot(current_scene);
			_element = new connector(x, y);
			current_scene->add_element(_element);

			wr2 = wr->split();
			port = current_scene->is_port(x,y);
			wr->add_port(port);
			wr2->add_port(port);
			port->add_wire(wr);
			port->add_wire(wr2);
			wr->reseat();
			wr2->reseat();
			wr->route(current_scene);
			wr2->route(current_scene);
			freshsplit = true;
			current_scene->cycle_color();
		}
		if (dragging)
			dragging->start_drag(x, y);

		if (!dragging && !freshsplit) {
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


		if (floating.size() > 0) {
			take_undo_snapshot(current_scene);
			for (auto flt : floating) {
				flt->reset_uuid();
				current_scene->add_element(flt);
				flt->stop_drag(this);
				flt->reseat();
			}
			floating.clear();
			if (active_icon)
				floating.push_back(active_icon->create_element());
			current_scene->cycle_color();
		}


	}
	if ( event.button.button == SDL_BUTTON_RIGHT) {
		class element *here = NULL;
		float x, y;
		dragging = NULL;

		clear_floating();
		if (active_icon)
			active_icon->set_inactive();
		active_icon = NULL;

		x = scr_to_X(event.motion.x);
		y = scr_to_Y(event.motion.y);
		for (auto elem:	current_scene->elements) {
			if (elem->intersect(x, y)) {
				here = elem;
			}
		}
		active_menu = NULL;
		if (here) {
			active_menu = here->get_menu();
		} else {
			class wire *wr = current_scene->is_wire(x, y);
			class port *is_port = current_scene->is_port(x,y);
			
			if (is_port) {
				active_menu = is_port->get_menu();
			} else if (!wr) {
				active_menu = current_scene->get_menu();
			}			
		}
		if (active_menu)
			active_menu->mouse_set(x, y);
		printf("Right button\n");
	}
	if (event.button.button == SDL_BUTTON_MIDDLE) {
		middle_X = event.motion.x;
		middle_Y = event.motion.y;
	}
	current_scene->remove_orphans();
	run_queued_calculations();
	return false;
}

bool canvas::handle_event_iconarea(SDL_Event &event)
{
	class icon *this_icon;
	
	this_icon = icon_bar->current_icon(event.motion.x, event.motion.y);
	active_menu = icon_bar->get_menu(event.motion.x, event.motion.y);
	
	if (active_menu)
			active_menu->mouse_set(scr_to_X(event.motion.x), scr_to_X(event.motion.y));
	
	if (this_icon)
		printf("ICON CLICK\n");
	if (active_icon && this_icon)
		active_icon->set_inactive();
	if (active_icon != this_icon && this_icon) {
		active_icon = this_icon;
		clear_floating();
	} else {
		if (this_icon)
			active_icon = NULL;
	}

	if (active_icon) {
		if (floating.size() == 0) {
			class element *elem;
			elem = active_icon->create_element();
			if (elem) {
				elem->place(mouseX, mouseY);
				elem->start_drag(mouseX, mouseY);	
				floating.push_back(elem);
			}
		}
	} else {
		clear_floating();
	}

	return false;
}


bool canvas::handle_event(SDL_Event &event)
{
	bool leave = false;
	bool someone_in_editmode = false;
	
	if (SDL_GetTicks64() - mouse_timestamp > 1000 && X_to_scr(mouseX) > main_area_rect.w && active_menu == NULL) 
		tooltip_eligable = true;
	else
		tooltip_eligable = false;


	if (event.type == EVENT_ZOOM_TO_FIT && event.user.data1 == current_scene) {  /* zoom to fit the screen */
		fittoscreen = true;
		float bX1 = current_scene->sizeX, bY1 = current_scene->sizeY, bX2 = 0, bY2 = 0;
		for (auto elem : current_scene->elements) {
			if (elem->get_X() < bX1)
				bX1 = elem->get_X();
			if (elem->get_Y() < bY1)
				bY1 = elem->get_Y();
			if (elem->get_X() + elem->get_width() > bX2)
				bX2 = elem->get_X() + elem->get_width();
			if (elem->get_Y() + elem->get_height() > bY2)
				bY2 = elem->get_Y() + elem->get_height();
		}

		if (bX2 != 0 && bX2 != bX1) {
			offsetX = bX1 - 3;
			offsetY = bY1 - 3;
			scaleX = main_area_rect.w / (bX2-bX1 + 6.0) ;
			scaleY = main_area_rect.h / (bY2-bY1 + 6.0) ;
			scaleX = std::min(scaleX, scaleY);
			scaleY = scaleX;
		}
	}
	
	
	for (auto elem : current_scene->elements)
		if (elem->in_edit_mode())
		   someone_in_editmode = true;
	switch (event.type) {
		case SDL_QUIT:
			printf("QUIT\n");
			leave = true;
			return true;
			break;

		case SDL_KEYDOWN:
			if ((event.key.keysym.mod & (KMOD_LSHIFT)))
				shift_down = true;
			else
				shift_down = false;
			switch (event.key.keysym.sym) {
			case SDLK_UP:
				offsetY--;
				if (offsetY < -1)
					offsetY = -1;
				fittoscreen = false;
				break;
			case SDLK_DOWN:
				offsetY++;
				if (offsetY > current_scene->sizeY + 1)
					offsetY = current_scene->sizeY;
				fittoscreen = false;
				break;
			case SDLK_LEFT:
				if (event.key.keysym.mod & KMOD_LCTRL) {
				     icon_bar->previous();
				} else {
					if (!someone_in_editmode) {
						offsetX--;
						if (offsetX < -1)
							offsetX = -1;
					}
					fittoscreen = false;
				}
				break;
					
			case SDLK_RIGHT:
				if (event.key.keysym.mod & KMOD_LCTRL) {
				     icon_bar->next();
				} else {
					if (!someone_in_editmode) {
						offsetX++;
						if (offsetX > current_scene->sizeX + 1)
							offsetX = current_scene->sizeX;
						fittoscreen = false;
					}
				}
				break;
			case SDLK_PLUS:
			case SDLK_KP_PLUS:
			case SDLK_EQUALS:
				if (!someone_in_editmode) {
					scaleX++;
					scaleY++;
					fittoscreen = false;
				}
				break;
			case SDLK_MINUS:
			case SDLK_KP_MINUS:
				if (!someone_in_editmode) {
					if (scaleX > 1)
						scaleX--;
					if (scaleY > 1)
						scaleY--;
					fittoscreen = false;
				}
				break;
			case SDLK_ESCAPE:
				clear_floating();
				active_menu = NULL;
				if (active_icon)
					active_icon->set_inactive();
				active_icon = NULL;
				current_scene->deselect_all();
				break;
			case SDLK_BACKSPACE:
				if (!someone_in_editmode) {
					take_undo_snapshot(current_scene);
					if (hover_wire) {
						hover_wire->remove();
						delete hover_wire;
						hover_wire = NULL;
					}
					current_scene->delete_selection();
					current_scene->process_delete_requests();
					current_scene->remove_orphans();
				}
				break;
			case SDLK_SPACE:
				if (!someone_in_editmode) {
					take_undo_snapshot(current_scene);
					for (auto elem : current_scene->elements) {
						if (!elem->is_selected())
							continue;
						elem->rotate_ports();
					}
				}
				break;
			case SDLK_g:
				if (!someone_in_editmode) {
					draw_grid = !draw_grid;
				}
				break;
			case SDLK_z:
				if (event.key.keysym.mod & KMOD_LCTRL) {
					printf("UNDO\n");
					class scene *oldscene;
					oldscene = swap_scene(get_undo());
					if (oldscene)
						delete(oldscene);
				}
				break;
			case SDLK_a:
				if (event.key.keysym.mod & KMOD_LCTRL) {
					for (auto elem : current_scene->elements) {
						elem->select();
					}
				}
				break;
			case SDLK_c:
				if (event.key.keysym.mod & KMOD_LCTRL) {
					printf("copy\n");
					json p;
					
					current_scene->selection_to_json(p);
					clipboard = p.dump();
					printf("clipboard is %s\n", clipboard.c_str());
				}
				break;
			case SDLK_q:
				if (event.key.keysym.mod & KMOD_LCTRL) {
					return true;
				}
				break;
			case SDLK_x:
				if (event.key.keysym.mod & KMOD_LCTRL) {
					json p;
					
 					take_undo_snapshot(current_scene);
					current_scene->selection_to_json(p);
					clipboard = p.dump();
					current_scene->delete_selection();
					current_scene->process_delete_requests();
					current_scene->remove_orphans();
				}
				break;
			case SDLK_v:
				if (event.key.keysym.mod & KMOD_LCTRL) {
					printf("paste\n");
					json p = json::parse(clipboard);
					from_json_to_floating(p);
					for (auto elem : floating) {
						elem->start_drag(mouseX, mouseY);
						elem->reseat();
					}
				}
				break;
			break;
		}
		case SDL_KEYUP:
			if ((event.key.keysym.mod & (KMOD_LSHIFT))) 
				shift_down = true;
			else
				shift_down = false;
			break;
		case SDL_MOUSEBUTTONDOWN:
			if (active_menu) {
				float x, y;
				x = scr_to_X(event.motion.x);
				y = scr_to_Y(event.motion.y);
				active_menu->mouse_click(x,y);
				active_menu = NULL;
				current_scene->process_delete_requests();
				break;
			}
			if (event.motion.x < main_area_rect.w) 
				canvas::handle_event_drawingarea(event);
			else
				canvas::handle_event_iconarea(event);
			break;
		case SDL_MOUSEBUTTONUP:
			if (event.button.button == SDL_BUTTON_LEFT) {
				left_mouse_down = false;
				float x, y;
				x = scr_to_X(event.motion.x);
				y = scr_to_Y(event.motion.y);
				if (in_area_select) {
					in_area_select = false;
					/* cause all the things inside to be selected */
					break;
				}
				if (dragging) {
					printf("Up\n");
					take_undo_snapshot(current_scene);
					if (!dragging->has_moved())  {
						printf("Click\n");
						if (!dragging->mouse_select(x,y)) {
							dragging->select();
							if (current_scene->selected_count() == 1)
								dragging->select_single();
						}
					}
					if (dragging->stop_drag(this))
						current_scene->rewire_section(dragging);

				}
				if (dragging_port && !current_scene->is_port(x, y)) {
					class wire *wr;
					class element *_element;

					take_undo_snapshot(current_scene);
					
					wr = current_scene->is_wire(x, y);
					if (wr) {
						class wire *wr2;
						class port *port;
						_element = new connector(x, y);
					        current_scene->add_element(_element);
					
						wr2 = wr->split();
						port = current_scene->is_port(x,y);
						if (!port)
							printf("No port found\n");
						else
							printf("Port %s found\n", port->name.c_str());
						wr->add_port(port);
						wr2->add_port(port);
						port->add_wire(wr);
						port->add_wire(wr2);
						wr->reseat();
						wr2->reseat();
						wr->route(current_scene);
						wr2->route(current_scene);
						printf("new split code\n");
					} else {
						printf("not new split code\n");
						_element = new connector(x, y);
					        current_scene->add_element(_element);
					}

					current_scene->cycle_color();
				}
				if (dragging_port && current_scene->is_port(x, y)) {
					class port *port2 = current_scene->is_port(x, y);
					
					if (dragging_port != port2) {
						take_undo_snapshot(current_scene);
						dragging_port->add_wire(dragging_wire);
						port2->add_wire(dragging_wire);
						dragging_wire->reseat();
						dragging_wire->route(current_scene);
						dragging_wire = NULL;
						printf("Connection made\n");

					}
				} 
				dragging = NULL;
				dragging_port = NULL;
				left_mouse_down = false;
				if (dragging_wire) {
					delete dragging_wire;
				}
				dragging_wire = NULL;
			}
			if (event.button.button == SDL_BUTTON_MIDDLE) {
				middle_X = event.motion.x;
				middle_Y = event.motion.y;
			}
			current_scene->remove_orphans();
			break;
		case SDL_MOUSEMOTION:
			float x,y;
			mouseX = scr_to_X(event.motion.x);
			mouseY = scr_to_Y(event.motion.y);
			mouse_timestamp = SDL_GetTicks64();
			x = scr_to_X(event.motion.x);
			y = scr_to_Y(event.motion.y);
			
			if (active_menu && !active_menu->mouse_in_bounds(x,y))
				active_menu = NULL;
				
			if (active_menu)
				active_menu->mouse_motion(x,y);

			if (in_area_select) {
				float X1,Y1,X2,Y2;
				if (area_select_X1 > mouseX){ 
					X1 = mouseX; 
					X2 = area_select_X1;
				} else {
					X1=  area_select_X1;
					X2 = mouseX;
				}

				if (area_select_Y1 > mouseY){ 
					Y1 = mouseY; 
					Y2 = area_select_Y1;
				} else {
					Y1= area_select_Y1;
					Y2 = mouseY;
				}

				current_scene->deselect_all();
				for (auto elem : current_scene->elements) {
					if (elem->get_X() >= X1 && elem->get_X()+elem->get_width() <= X2 && elem->get_Y() >= Y1 && elem->get_Y()+elem->get_height() <= Y2)
						elem->select();
				}
			}
			
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
			for (auto flt : floating) {
				flt->update_drag(this, current_scene,
						      scr_to_X(event.motion.x),
						      scr_to_Y(event.motion.y));
			}
			if (dragging_wire) {
				bool banned = false;
				for (auto elem:	current_scene->elements) {
					if (elem->intersect(x, y) && !elem->is_port(x,y))
						banned = true;
				}
				if (!banned)
					dragging_wire->move_target(mouseX, mouseY);
			}

			if (event.motion.state & SDL_BUTTON_MMASK) {
				float fromX, fromY, toX, toY;
				fromX = scr_to_X(middle_X);
				fromY = scr_to_Y(middle_Y);
				toX = scr_to_X(event.motion.x);
				toY = scr_to_Y(event.motion.y);
				offsetX -= (toX - fromX);
				offsetY -= (toY - fromY);

				middle_X = event.motion.x;
				middle_Y = event.motion.y;
				fittoscreen = false;

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
				fittoscreen = false;
				
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
				fittoscreen = false;
				/* need to keep the point under the cursor the same */
				while (scr_to_X(cX) > mouseX && count++ < 1000)  
					offsetX -= 0.05;
				while (scr_to_Y(cY) > mouseY && count++ < 1000)
					offsetY -= 0.05;
			}
			
			break;

		case SDL_POLLSENTINEL:
			break;
		case SDL_WINDOWEVENT:
			switch( event.window.event) {
			case SDL_WINDOWEVENT_SHOWN:
				window_shown = true;
			        break;
			            
		        case SDL_WINDOWEVENT_HIDDEN:
		        	window_shown = false;
		        	break;
			case SDL_WINDOWEVENT_RESIZED:
				main_area_rect.w = event.window.data1 - 220;;
				main_area_rect.h = event.window.data2;
				ui_area_rect.x = main_area_rect.w;
				ui_area_rect.y = 0;
				ui_area_rect.w = event.window.data1 - main_area_rect.w;
				ui_area_rect.h = event.window.data2;
				icon_bar->resize(ui_area_rect);
				if (fittoscreen)
					callback_fit_to_screen(current_scene);
				break;
			case SDL_WINDOWEVENT_CLOSE:   /* user clicked the 'x' */
				SDL_HideWindow(window);
				leave = true;
				break;
			}
			break;
		}
		
	for (auto elem : current_scene->elements)
		elem->handle_event(this, event);
	run_queued_calculations();
	current_scene->remove_orphans();
	return leave;
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
			draw_box(X + 0.2, Y + 0.2, X + 9.8, Y + 9.8,
				COLOR_BACKGROUND_MAIN);
			X = X + 10;
		}
		Y = Y + 10;
	}
	
	
	if (draw_grid) {
		int x, y;
		class wiregrid * grid = new wiregrid(current_scene->sizeX, current_scene->sizeY);
		current_scene->fill_grid(grid);
		
		for (y = 0; y < current_scene->sizeY ; y++)
			for (x = 0; x < current_scene->sizeX ; x++) {
				float c = grid->get_soft_cost(x, y);
				if (c > 0.01) {
					draw_box(x + 0.1, y + 0.1, x + 0.9, y + 0.9, COLOR_ELEMENT_GHOST, 64 * c);
				}
				if (grid->is_blocked(x,y)) {
					draw_box(x + 0.1, y + 0.1, x + 0.9, y + 0.9, COLOR_ELEMENT_DND);
				} 
//				if (current_scene->is_port(x,y)) {
//					draw_circle(x + 0.5, y + 0.5, 2, COLOR_VALUE_RED);
//				}
			}
		
		delete grid;
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
	for (auto flt : floating) {
		flt->draw(this, DRAW_GHOST);
		flt->draw(this, DRAW_DND);
	}

	if (dragging_wire) {
		class port *port2;
		int color = COLOR_WIRE_INVALID;

		port2 = current_scene->is_port(mouseX, mouseY);
		if (port2)
			color = COLOR_WIRE_SOLID;

		dragging_wire->draw(this, color);
	}

	for (auto const elem: current_scene->elements) {
		if (elem != dragging)
			elem->draw_phase2(this, DRAW_NORMAL);
	}


	if (in_area_select) {
		float X1,Y1,X2,Y2;
		if (!area_select_texture)
			area_select_texture = load_image("assets/area_select.png");

		if (area_select_X1 > mouseX){ 
			X1 = mouseX; 
			X2 = area_select_X1;
		} else {
			X1= area_select_X1;
			X2 = mouseX;
		}

		if (area_select_Y1 > mouseY){ 
			Y1 = mouseY; 
			Y2 = area_select_Y1;
		} else {
			Y1= area_select_Y1;
			Y2 = mouseY;
		}

		draw_image(area_select_texture, X1, Y1, X2-X1, Y2-Y1, 64);
	}
	
	if (tooltip_eligable) {
		std::string tooltip = icon_bar->current_tooltip(X_to_scr(mouseX), X_to_scr(mouseY));
		
		if (tooltip != "") {
			draw_tooltip(mouseX, mouseY, tooltip);
		}
	}
	
	if (active_menu) {
		SDL_RenderSetClipRect(renderer, NULL);
		active_menu->draw_at(this);
	}

	if (window_shown)
		SDL_RenderPresent(renderer);
}

void canvas::draw_tooltip(float X, float Y, std::string tooltip)
{
	SDL_Texture *text, *shade;
	float scale = 1.5;
	
	text = text_to_texture(tooltip)	;
	shade = load_image("assets/gray.png");
	if (!text)
		return;
		
	SDL_Rect rect;
	SDL_Point size;
	int x, y;
	
	SDL_QueryTexture(text, NULL, NULL, &size.x, &size.y);

	x = X_to_scr(X) - size.x/scale;
	y = Y_to_scr(Y) - size.y/scale;

	rect.x = x;
	rect.y = y;
	rect.w = size.x/scale;
	rect.h = size.y/scale;

	SDL_RenderSetClipRect(renderer, NULL);	
	SDL_RenderCopy(renderer, shade, NULL, &rect);
	SDL_RenderCopy(renderer, text, NULL, &rect);
}



void canvas::to_json(json& j)
{
	j["offsetX"] = offsetX;
	j["offsetY"] = offsetY;
	j["scaleX"] = scaleX;
	j["scaleY"] = scaleY;
}

class scene * canvas::swap_scene(class scene *scene)
{
	class scene *tmp;

	if (!scene)
		return NULL;
	tmp = current_scene;
	current_scene = scene;
	return tmp;
}

#define UNDO_DEPTH 16



static std::vector<std::string> undo_list;

void canvas::take_undo_snapshot(class scene *scene)
{
	json J;
	std::string s;

	scene->to_json(J);

	s = J.dump();

	if (undo_list.size()== 0 || s != undo_list[undo_list.size() -1])
		undo_list.push_back(s);
	if (undo_list.size() > UNDO_DEPTH)
		undo_list.erase(undo_list.begin() + 0);
}


class scene *canvas::get_undo(void)
{
	class scene *scene;
	std::string s;
	json J;
	
	clear_wire_factory();

	if (undo_list.size() == 0)
		return NULL;

	scene = new class scene(current_scene->get_name());
	s = undo_list[undo_list.size() -1];

	undo_list.pop_back();
	J = json::parse(s);
	scene->from_json(J);
	return scene;
}

void canvas::from_json_to_floating(json &j)
{
	unsigned int i;
	bool saved = wire_factory_force_new_name;

  	wire_factory_force_new_name = true;
	clear_wire_factory();

	for (i = 0; i <j["elements"].size(); i++) {
		json p = j["elements"][i];
		std::string cid = p["class_id"];
		class element *element = element_from_class_id(cid);
		element->from_json(p);
		floating.push_back(element);
	}
	clear_wire_factory();
	wire_factory_force_new_name = saved;

}


void canvas::update_window_title(void)
{
	std::string s;
	
	s = "GridCad " + current_scene->get_full_name();
	SDL_SetWindowTitle(window, s.c_str());
}

float canvas::distance_from_mouse(float X, float Y)
{
	return sqrtf( (X-mouseX)*(X-mouseX) + (Y-mouseY)*(Y-mouseY));
}

void canvas::clear_floating(void)
{
	for (auto elem : floating)
		delete elem;
	floating.clear();
}

float canvas::screen_width(void)
{
	float Wpixels = ui_area_rect.x + ui_area_rect.w;
	float W = scr_to_X(Wpixels);
	
	return W;
}