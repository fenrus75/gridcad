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
#include "canvas.h"
#include "scene.h"
#include "element.h"
#include "connector.h"
#include "wire.h"
#include "port.h"
#include "iconbar.h"
#include "contextmenu.h"
#include "buttonbar.h"
#include "dialog.h"
#include "synth.h"

#include <nlohmann/json.hpp>
#include <sys/time.h>

bool wire_debug_mode = false;
bool show_fps = false;
float  animation_time = 400;

using json = nlohmann::json;

extern void callback_fit_to_screen(class scene *scene);
extern void callback_fit_to_screen2(class scene *scene);
extern void callback_autoclock(class scene *scene);
extern void callback_reroute(class scene *scene);

static std::string clipboard;

static float distsq(float X1, float Y1, float X2, float Y2)
{
	return (X1-X2)*(X1-X2) + (Y1-Y2)*(Y1-Y2);
}

canvas::canvas(class scene *_scene, struct capabilities *cap)
{
	std::string s;

	if (cap) 
		show_toolchain = cap->has_full_workflow;

	s = "GridCad " + _scene->get_full_name();
	window =
		SDL_CreateWindow(s.c_str(), SDL_WINDOWPOS_UNDEFINED,
				SDL_WINDOWPOS_UNDEFINED, 1024, 768, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	//	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

	windowID = SDL_GetWindowID(window);
	SDL_GetDisplayDPI(0, &DPI, NULL, NULL);
	if (DPI < 96)
		DPI = 96;


	button_rect.x = 0;
	button_rect.y = 0;
	button_rect.w = DPI/2;
	button_rect.h = 768;
	ui_area_rect.x = 820;
	ui_area_rect.y = 0;
	ui_area_rect.w = 1024 - 820;
	ui_area_rect.h = 768;
	main_area_rect.x = button_rect.w;
	main_area_rect.y = 0;
	main_area_rect.w = 1024 - ui_area_rect.w - button_rect.w;
	main_area_rect.h = 768;

	scaleX = 25;
	scaleY = 25;

	dragging = NULL;
	left_mouse_down = false;
	dragging_port = NULL;
	dragging_wire = NULL;
	icon_bar = new iconbar(renderer, ui_area_rect, this);
	button_bar = new buttonbar(this, button_rect.w);
	button_bar->add_button("Save design to file", "assets/save_icon.png", EVENT_SAVE);
	button_bar->add_button("Fit to screen", "assets/icon_fit_to_screen.png", EVENT_ZOOM_TO_FIT);
	button_bar->add_button("Wire all clock signals", "assets/autoclock.png", EVENT_AUTOCLOCK);
	button_bar->add_button("Redo all wiring", "assets/icon_rewire.png", EVENT_REWIRE);
	if (show_toolchain) {
		button_bar->add_button("Compile verilog", "assets/icon_compile_verilog.png", EVENT_RUN_VERILOG);
		button_bar->add_button("Upload to FPGA", "assets/icon_program_fpga.png", EVENT_PROGRAM_FPGA);
	}
	current_scene = _scene;
	callback_fit_to_screen2(current_scene);
	SDL_MaximizeWindow(window);
}


canvas::~canvas(void)
{
	printf("Canvas destructor\n");
	zap_autocomplete();
	unregister_canvas(this);
	SDL_DestroyRenderer(renderer);
	renderer = NULL;
	SDL_DestroyWindow(window);
	delete icon_bar;  icon_bar = NULL;
	delete current_scene;
	delete button_bar; button_bar = NULL;
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
				//				printf("Start drag: %5.2f %5.2f \n", x, y);
				if (dragging == NULL || dragging->is_background())
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

		if (!dragging && wr && !is_port && floating.size()==0) {
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
			current_scene->redo_nets();
		}

		if (!freshsplit) {
			dragging_port = current_scene->is_port(x, y);
			if (dragging && dragging->is_background() == false)
				dragging_port = NULL;
			if (dragging_port) {
				dragging_port->screenX = x;
				dragging_port->screenY = y;

				best_port_to_autocomplete(dragging_port);

				if (dragging_wire)
					delete dragging_wire;
				dragging_wire =new wire(floorf(x), 
							floorf(y),
							floorf(x),
							floorf(y));
				dragging = NULL;
			}
		}

		if (dragging)
			dragging->start_drag(x, y);


		if (floating.size() > 0) {
			take_undo_snapshot(current_scene);
			for (auto flt : floating) {
				flt->reset_uuid();
				current_scene->add_element(flt);
				flt->stop_drag(this);
				flt->reseat();
				current_scene->rewire_section(flt);
			}
			floating.clear();

			if (active_icon) {
				if (shift_down) {
					floating.push_back(active_icon->create_element());
				} else {
					active_icon->set_inactive();
					active_icon = NULL;
				}
			}
		
			current_scene->cycle_color();
			current_scene->redo_nets();
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
				if (here == NULL || elem->is_background() == false) {
					printf("HERE %s\n", elem->class_id().c_str());
					here = elem;
				}
			}
		}
		active_menu = NULL;
		class wire *wr = current_scene->is_wire(x, y);
		class port *is_port = current_scene->is_port(x,y);


		if (here) {
			active_menu = here->get_menu();
		} 

		if (!here || here->is_background())  {
			if (is_port) {
				active_menu = is_port->get_menu();
			} else if (!wr && !here) {
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
		float x,y;
		x = scr_to_X(event.motion.x);
		y = scr_to_Y(event.motion.y);

		class element *here = NULL;
		for (auto elem:	current_scene->elements) {
			if (elem->intersect(x, y) && elem->class_id() == "connector:") {
				here = elem;
			}
		}

		if (here) {
			printf("Setting dragging for MM\n");
			dragging = here;
			dragging->start_drag(x, y);
		}
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


	/* set the mouse position -- but we fake it to be the left of the menu area for better menu placement */
	if (active_menu)
		active_menu->mouse_set(scr_to_X(ui_area_rect.x+15), scr_to_Y(event.motion.y));

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

	if (SDL_GetTicks64() - mouse_timestamp > 1000 && active_menu == NULL) 
		tooltip_eligable = true;
	else
		tooltip_eligable = false;

	if (event.type == EVENT_RELOAD_ICONBAR)
		icon_bar->create_menu();


	if (event.type == EVENT_AUTOCLOCK && event.user.data1 == current_scene) {
		take_undo_snapshot(current_scene);
		callback_autoclock(current_scene);
	}


	if (event.type == EVENT_REWIRE && event.user.data1 == current_scene) {
		take_undo_snapshot(current_scene);
		callback_reroute(current_scene);
	}

	if (event.type == EVENT_RUN_VERILOG && event.user.data1 == current_scene) {
		class synth *dialog = new class synth(main_area_rect.w + ui_area_rect.w + button_rect.w, main_area_rect.h, projectname, "");

		set_dialog(dialog);
	}
	if (event.type == EVENT_PROGRAM_FPGA && event.user.data1 == current_scene) {
		class synth *dialog = new class synth(main_area_rect.w + ui_area_rect.w + button_rect.w, main_area_rect.h, projectname, "program");

		set_dialog(dialog);
	}

	if (event.type == EVENT_CLOSE_DIALOG && event.user.data1 == current_scene) {
		if (dialogbox)
			delete dialogbox;
		dialogbox = NULL;
	}

	if (event.type == EVENT_SEQUENCER_RESET && event.user.data1 == current_scene) {
		for (auto elem : current_scene->elements)
			elem->reset_pointer();
	}
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
			from_offsetX = offsetX;
			from_offsetY = offsetY;
			from_scaleX = scaleX;
			from_scaleY = scaleY;
			animation_start = SDL_GetTicks64();
			target_offsetX = bX1 - 3;
			target_offsetY = bY1 - 3;
			target_scaleX = main_area_rect.w / (bX2-bX1 + 6.0) ;
			target_scaleY = main_area_rect.h / (bY2-bY1 + 6.0) ;
			target_scaleX = std::min(target_scaleX, target_scaleY);
			target_scaleY = target_scaleX;
			in_animation = true;
		
			if (event.user.code != 0) {
				from_offsetX = target_offsetX;
				from_offsetY = target_offsetY;
				from_scaleX = target_scaleX;
				from_scaleY = target_scaleY;
			}
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
			if (event.key.keysym.sym != SDLK_TAB)
				zap_autocomplete();
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
						current_scene->redo_nets();
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
				case SDLK_TAB:
					if (!someone_in_editmode) {
						if (autocomplete.size() > 0)
							take_undo_snapshot(current_scene);
						apply_autocomplete();
						if (dragging_wire) {
							delete dragging_wire;
							dragging_wire = NULL;
							dragging_port = NULL;
						}

					}
					break;
				case SDLK_f:
					if (!someone_in_editmode) {
						show_fps = !show_fps;
					}
					break;
				case SDLK_g:
					if (!someone_in_editmode) {
						draw_grid = !draw_grid;
					}
					break;
				case SDLK_o:
					if (!someone_in_editmode) {
						if (animation_time > 1 && animation_time < 1000)
							animation_time = 4000;
						else if (animation_time == 1)
							animation_time = 400;
						else
							animation_time = 1;

					}
					break;
				case SDLK_n:
					if (!someone_in_editmode) {
						wire_debug_mode = !wire_debug_mode;
					}
					break;
				case SDLK_z:
					if (event.key.keysym.mod & KMOD_LCTRL) {
						zap_autocomplete();
						printf("UNDO\n");
						class scene *oldscene;
						oldscene = swap_scene(get_undo());
						if (oldscene)
							delete(oldscene);
						current_scene->redo_nets();
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
						current_scene->redo_nets();
					}
					break;
				case SDLK_v:
					if (event.key.keysym.mod & KMOD_LCTRL) {
						float x1 = 1000000, x2 = -100000;
						float y1 = 1000000, y2 = -100000;
						printf("paste\n");
						json p = json::parse(clipboard);
						from_json_to_floating(p);
						for (auto elem : floating) {
							if (elem->get_X() < x1)
								x1 = elem->get_X();
							if (elem->get_X() + elem->get_width() > x2)
								x2 = elem->get_X() + elem->get_width();
							if (elem->get_Y() < y1)
								y1 = elem->get_Y();
							if (elem->get_Y() + elem->get_height() > y2)
								y2 = elem->get_Y() + elem->get_height();
						}
						for (auto elem : floating) {
							elem->start_drag((x1+x2)/2, (y1+y2)/2);
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
			zap_autocomplete();
			if (active_menu && !dialogbox) {
				float x, y;
				x = scr_to_X(event.motion.x);
				y = scr_to_Y(event.motion.y);
				active_menu->mouse_click(x,y);
				active_menu = NULL;
				current_scene->process_delete_requests();
				break;
			}
			if (event.motion.x < main_area_rect.w + main_area_rect.x && event.motion.x >= main_area_rect.x && !dialogbox) 
				canvas::handle_event_drawingarea(event);
			else
				if (event.motion.x >= ui_area_rect.x && !dialogbox)
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

					current_scene->redo_nets();
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
						_element->reroute_all_wires();
						printf("new split code\n");
					} else {
						printf("not new split code\n");
						_element = new connector(x, y);
						current_scene->add_element(_element);
					}

					current_scene->cycle_color();
					current_scene->redo_nets();
				}
				if (dragging_port && current_scene->is_port(x, y)) {
					class port *port2 = current_scene->is_port(x, y);

					if (dragging_port != port2) {
						take_undo_snapshot(current_scene);
						zap_autocomplete();
						dragging_port->add_wire(dragging_wire);
						port2->add_wire(dragging_wire);
						dragging_wire->reseat();
						dragging_wire->route(current_scene);
						dragging_wire = NULL;
						printf("Connection made\n");

						current_scene->redo_nets();
						create_autocomplete_from_wire(dragging_port, port2);
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
				float x, y;
				x = scr_to_X(event.motion.x);
				y = scr_to_Y(event.motion.y);
				middle_X = event.motion.x;
				middle_Y = event.motion.y;
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
					dragging = NULL;

				}
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
					if (elem->intersect(x, y) && !elem->is_port(x,y) && elem->is_background() == false)
						banned = true;
				}
				if (!banned)
					dragging_wire->move_target(mouseX, mouseY);
			}

			if (event.motion.state & SDL_BUTTON_MMASK && !dragging) {
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
			if (dialogbox)
				break;
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
					main_area_rect.w = event.window.data1 - 220 - button_rect.w;
					main_area_rect.h = event.window.data2;
					ui_area_rect.x = main_area_rect.w + button_rect.w;
					ui_area_rect.y = 0;
					ui_area_rect.w = event.window.data1 - ui_area_rect.x;
					ui_area_rect.h = event.window.data2;
					icon_bar->resize(ui_area_rect);
					if (fittoscreen)
						callback_fit_to_screen2(current_scene);
					if (dialogbox)
						dialogbox->update_screen_size(event.window.data1, event.window.data2);
					break;
				case SDL_WINDOWEVENT_CLOSE:   /* user clicked the 'x' */
					SDL_HideWindow(window);
					leave = true;
					break;
				case SDL_WINDOWEVENT_FOCUS_GAINED:
					has_focus = true;
					break;
				case SDL_WINDOWEVENT_FOCUS_LOST:
					has_focus = false;
					break;
			}
			break;
	}

	for (auto elem : current_scene->elements)
		elem->handle_event(this, event);
	button_bar->handle_event(event);
	icon_bar->handle_event(event);
	if (dialogbox)
		dialogbox->handle_event(this, event);
	run_queued_calculations();
	current_scene->remove_orphans();
	return leave;
}

void canvas::draw(void)
{
	if (!window_shown)
		return;
		
	/* first, draw the lighter gray background */

	SDL_SetRenderDrawColor(renderer, R(COLOR_BACKGROUND_GRID),
			G(COLOR_BACKGROUND_GRID),
			B(COLOR_BACKGROUND_GRID),
			Alpha(COLOR_BACKGROUND_GRID));
	SDL_RenderClear(renderer);
	basecanvas::draw();
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
			elem->draw_early(this, DRAW_NORMAL);
		else
			elem->draw_early(this, DRAW_ORIGIN);
	}

	for (auto const elem: current_scene->elements) {
		if (elem != dragging)
			elem->draw(this, DRAW_NORMAL);
		else
			elem->draw(this, DRAW_ORIGIN);
	}

	if (dragging) {
		dragging->draw_early(this, DRAW_GHOST);
		dragging->draw_early(this, DRAW_DND);
		dragging->draw(this, DRAW_GHOST);
		dragging->draw(this, DRAW_DND);
	}
	for (auto flt : floating) {
		flt->draw_early(this, DRAW_GHOST);
		flt->draw_early(this, DRAW_DND);
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

	SDL_RenderSetClipRect(renderer, NULL);
	if (fps > 0)
		button_bar->set_fps(fps);
	button_bar->draw_at(this, button_rect.w, button_rect.h);

	if (tooltip_eligable) {
		std::string tooltip = "";
		if (X_to_scr(mouseX) > main_area_rect.x + main_area_rect.w) {
			tooltip = icon_bar->current_tooltip(X_to_scr(mouseX), Y_to_scr(mouseY));
		}
		if (X_to_scr(mouseX) < main_area_rect.x) {
			tooltip = button_bar->current_tooltip(X_to_scr(mouseX), Y_to_scr(mouseY));
		}		
		if (X_to_scr(mouseX) >= main_area_rect.x && X_to_scr(mouseX) <= ui_area_rect.x) {
			class port *prt = current_scene->is_port(mouseX,mouseY);
			if (prt)
				tooltip = prt->get_tooltip();

		}
		if (tooltip != "") {
			draw_tooltip(mouseX, mouseY, tooltip);
		}
	}

	draw_autocomplete();

	if (active_menu) {
		SDL_RenderSetClipRect(renderer, NULL);
		active_menu->draw_at(this);
	}

	if (dialogbox)
		dialogbox->draw(this);

	SDL_RenderPresent(renderer);
}

static float scale = -1;

static void fill_scale(void)
{
	if (scale > 0)
		return;
	float dpi;
	SDL_GetDisplayDPI(0, &dpi, NULL, NULL);
	if (dpi < 90)
		dpi = 96;
	scale = 1.3 * 96.0 / dpi;
}


void canvas::draw_tooltip(float X, float Y, std::string tooltip)
{
	SDL_Texture *text, *shade;
	fill_scale();

	text = text_to_texture(tooltip)	;
	shade = load_image("assets/gray.png");
	if (!text)
		return;

	SDL_Rect rect;
	SDL_Point size;
	int x, y;

	SDL_QueryTexture(text, NULL, NULL, &size.x, &size.y);

	x = X_to_scr(X);
	if (x > size.x/scale) 
		x = x - size.x/scale;
	y = Y_to_scr(Y);
	if (y > size.y/scale)
		y = y - size.y/scale;


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

void canvas::zap_autocomplete(void)
{
	for (auto a : autocomplete) {
		if (a->tempwire)
			delete a->tempwire;
		delete a;
	}
	autocomplete.clear();
}

static std::string alpha_begin(std::string input)
{
	std::string s;

	for (unsigned i = 0; i < input.size(); i++)
	{
		if (input[i] >= '0' || input[i] <= '1')
			break;
		s = s + input[i];
	}
	return s;
}
static std::string numeric_end(std::string input)
{
	std::string s;

	for (unsigned i = 0; i < input.size(); i++)
	{
		if (input[i] >= '0' || input[i] <= '1')
			s = s + input[i];
		else 
			s = "";
	}
	return s;
}

void canvas::draw_autocomplete(void)
{
	for (auto autoc : autocomplete) 
		if (autoc->tempwire) {
			autoc->tempwire->draw(this, COLOR_ELEMENT_GHOST);
		}
}

void canvas::create_autocomplete_from_wire(class port *first, class port *second)
{
	class element *frome = NULL, *toe = NULL;
	int from_port_index = -100;
	int to_port_index = -100;

	zap_autocomplete();

	printf("Creating multiwire autocomplete\n");	


	for (auto elem : current_scene->elements) {
		if (elem->has_port(first) >= 0) {
			from_port_index = elem->has_port(first);
			frome = elem;
		}
		if (elem->has_port(second) >= 0) {
			to_port_index = elem->has_port(second);
			toe = elem;
		}
	}

	if (from_port_index < 0 || to_port_index < 0) {
		printf("Couldn't find both ports\n");
		return;
	}

	from_port_index++;
	to_port_index++;

	while (true) {
		class port *candidate_from, *candidate_to;

		candidate_from = frome->port_at(from_port_index);
		if (!candidate_from)
			break;
		candidate_to = toe->port_at(to_port_index);
		if (!candidate_to)
			break;

		if (alpha_begin(first->name) != alpha_begin(candidate_from->name))
			break;
		if (alpha_begin(second->name) != alpha_begin(candidate_to->name))
			break;

		if (numeric_end(candidate_to->name) != numeric_end(candidate_from->name))
			break;

		if (candidate_to->direction != second->direction)
			break;
		if (candidate_from->direction != first->direction)
			break;


		class autocomplete_element *autoc = new class autocomplete_element();
		autoc->from = frome;
		autoc->to = toe;
		autoc->from_port = candidate_from;
		autoc->to_port = candidate_to;

		if (candidate_to->direction == PORT_IN)
			autoc->tempwire = new class wire(autoc->to_port->screenX, autoc->to_port->screenY, autoc->from_port->screenX, autoc->from_port->screenY, 0);
		else
			autoc->tempwire = new class wire(autoc->from_port->screenX, autoc->from_port->screenY, autoc->to_port->screenX, autoc->to_port->screenY, 0);
		autocomplete.push_back(autoc);

		from_port_index++;
		to_port_index++;
	};

	printf("Creating autocomplete of size %lu\n", autocomplete.size());


}

void canvas::apply_autocomplete(void)
{
	class port *p1 = NULL, *p2 = NULL;
	for (auto autoc : autocomplete) {
		class wire *wire = autoc->tempwire;
		autoc->tempwire = NULL;

		wire->add_port(autoc->from_port);
		wire->add_port(autoc->to_port);
		wire->clear_route();
		autoc->from_port->add_wire(wire);
		autoc->to_port->add_wire(wire);
		p1 = autoc->from_port;
		p2 = autoc->to_port;

	}


	if (autocomplete.size() == 1) {
		zap_autocomplete();
		create_autocomplete_from_wire(p1, p2);
	} else {
		zap_autocomplete();
	}

}

static bool direction_compatible(class port *one, class port *two)
{
	if (one->direction == PORT_IN && two->direction == PORT_IN)
		return false;
	if (one->direction == PORT_OUT && two->direction == PORT_OUT)
		return false;


	return true;
}

static bool width_compatible(class port *one, class port *two)
{
	if (one->get_width() == two->get_width())
		return true;
	if (one->get_width() == 0)
		return true;
	if (two->get_width() == 0)
		return true;

	return false;
}

void canvas::best_port_to_autocomplete(class port *origin)
{
	class port *target = NULL;
	class element *targete = NULL;
	class element *sourcee = NULL;
	double target_dist = 500*500*1000;

	zap_autocomplete();

	for (auto elem : current_scene->elements) {
		unsigned int i = 0;
		class port *p = NULL;
		do {
			p = elem->port_at(i);
			if (!p)
				break;
			i++;
			if (p == origin)
				sourcee = elem;
		} while(p);
	}

	for (auto elem : current_scene->elements) {
		unsigned int i = 0;
		class port *p = NULL;
		do {	
			float dist;
			p = elem->port_at(i);
			if (!p)
				break;
			i++;
			if (p == origin)
				sourcee = elem;
			if (p->has_wires())
				continue;
			if (!direction_compatible(origin, p))
				continue;
			if (!width_compatible(origin, p))
				continue;

			if (sourcee == elem)
				continue;

			dist = distsq(origin->screenX, origin->screenY, p->screenX, p->screenY);

			/* 50% discount of the ports have the same name */
			if (origin->name == p->name)
				dist = dist/2;

			if (dist < target_dist) {
				target_dist = dist;
				target = p;
				targete = elem;
			}

		} while (p);
	}
	if (target == NULL || sourcee == NULL || targete == NULL)
		return;
	class autocomplete_element *autoc = new class autocomplete_element();
	autoc->from = sourcee;
	autoc->to = targete;
	autoc->from_port = origin;
	autoc->to_port = target;
	if (target->direction == PORT_IN)
		autoc->tempwire = new class wire(autoc->to_port->screenX, autoc->to_port->screenY, autoc->from_port->screenX, autoc->from_port->screenY, 0);
	else
		autoc->tempwire = new class wire(autoc->from_port->screenX, autoc->from_port->screenY, autoc->to_port->screenX, autoc->to_port->screenY, 0);
	autocomplete.push_back(autoc);


}
