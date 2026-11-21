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
#include "timelinebar.h"
#include "vcd_session.h"
#include "placer.h"
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

canvas::canvas(std::unique_ptr<class scene> _scene, struct capabilities *cap)
{
	std::string s;

	if (cap) 
		show_toolchain = cap->has_full_workflow;

	s = "GridCad " + _scene->get_full_name();
	window.reset(
		SDL_CreateWindow(s.c_str(), SDL_WINDOWPOS_UNDEFINED,
				SDL_WINDOWPOS_UNDEFINED, 1024, 768, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI));
	renderer.reset(SDL_CreateRenderer(window.get(), -1, SDL_RENDERER_ACCELERATED));
	SDL_SetRenderDrawBlendMode(renderer.get(), SDL_BLENDMODE_BLEND);

	windowID = SDL_GetWindowID(window.get());
	SDL_GetDisplayDPI(0, &DPI, nullptr, nullptr);
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

	dragging = nullptr;
	left_mouse_down = false;
	dragging_port = nullptr;
	dragging_wire.reset();
	icon_bar = std::make_unique<iconbar>(renderer.get(), ui_area_rect, this);
	button_bar = std::make_unique<buttonbar>(this, button_rect.w);
	timeline_bar = std::make_unique<timelinebar>();
	timeline_bar->set_session(get_active_vcd_session());
	button_bar->add_button("Save design to file", "assets/save_icon.png", EVENT_SAVE);
	button_bar->add_button("Fit to screen", "assets/icon_fit_to_screen.png", EVENT_ZOOM_TO_FIT);
	button_bar->add_button("Wire all clock signals", "assets/autoclock.png", EVENT_AUTOCLOCK);
	button_bar->add_button(is_import_view_mode() ? "Redo connectors & all wiring" : "Redo all wiring",
	                       "assets/icon_rewire.png", EVENT_REWIRE);
	if (show_toolchain) {
		button_bar->add_button("Compile verilog", "assets/icon_compile_verilog.png", EVENT_RUN_VERILOG);
		button_bar->add_button("Upload to FPGA", "assets/icon_program_fpga.png", EVENT_PROGRAM_FPGA);
	}
	current_scene = std::move(_scene);
	if (current_scene)
		current_scene->maybe_run_steiner_routing();
	callback_fit_to_screen2(current_scene.get());
	SDL_MaximizeWindow(window.get());
}


autocomplete_element::~autocomplete_element(void) = default;

void canvas::set_dialog(std::unique_ptr<class dialog> D)
{
	dialogbox = std::move(D);
}

void canvas::set_vcd_session(class vcd_session *sess)
{
	if (timeline_bar)
		timeline_bar->set_session(sess);
}

canvas::~canvas(void)
{
	logger::get().debug("Canvas destructor");
	zap_autocomplete();
	unregister_canvas(this);
}

bool canvas::handle_event_drawingarea(SDL_Event &event)
{
	bool freshsplit = false;
	if (!shift_down)
		current_scene->deselect_all();
	if (!left_mouse_down
			&& event.button.button == SDL_BUTTON_LEFT) {

		std::shared_ptr<class wire> wr;
		bool is_port = false;
		//		bool is_element = false;
		bool is_wire = false;

		/* priority order port > element > wire */

		if (dragging)
			dragging->deselect();
		dragging = nullptr;
		dragging_port = nullptr;
		left_mouse_down = true;
		float x, y;

		click_start_X = event.motion.x;
		click_start_Y = event.motion.y;

		x = scr_to_X(event.motion.x);
		y = scr_to_Y(event.motion.y);

		is_port = current_scene->is_port(x,y);

		for (auto &elem : current_scene->elements) {
			if (elem->intersect(x, y) && !elem->is_port(x,y)) {
				if (dragging == nullptr || dragging->is_background())
					dragging = elem.get();
			}
		}


		wr = current_scene->is_wire(x, y);
		if (wr)
			is_wire = true;
		if (is_wire)
			logger::get().debug("WR  {}", dragging != nullptr);


		if (!dragging && !is_wire && floating.size() == 0 && !is_port) {
			in_area_select = true;
			area_select_X1 = x;
			area_select_Y1 = y;	
			return false;
		}

		if (!dragging && wr && !is_port && floating.size()==0) {
			class port *port;
			take_undo_snapshot(current_scene.get());
			current_scene->mark_layout_dirty();
			current_scene->add_element(std::make_unique<connector>(x, y));

			auto wr2 = wr->split();
			port = current_scene->is_port(x,y);
			wr->add_port(port);
			wr2->add_port(port);
			port->add_wire(wr);
			port->add_wire(wr2);
			wr->reseat();
			wr2->reseat();
			wr->route(current_scene.get());
			wr2->route(current_scene.get());
			freshsplit = true;
			current_scene->cycle_color();
			current_scene->redo_nets();
		}

		if (!freshsplit) {
			dragging_port = current_scene->is_port(x, y);
			if (dragging && dragging->is_background() == false)
				dragging_port = nullptr;
			if (dragging_port) {
				dragging_port->screenX = x;
				dragging_port->screenY = y;

				best_port_to_autocomplete(dragging_port);

				dragging_wire = std::make_shared<wire>(floorf(x), 
							floorf(y),
							floorf(x),
							floorf(y));
				dragging = nullptr;
			}
		}

		if (dragging)
			dragging->start_drag(x, y);


		if (floating.size() > 0) {
			take_undo_snapshot(current_scene.get());
			current_scene->mark_layout_dirty();
			for (auto &flt : floating) {
				class element *raw = flt.get();
				raw->reset_uuid();
				current_scene->add_element(std::move(flt));
				raw->stop_drag(this);
				raw->reseat();
				current_scene->rewire_section(raw);
			}
			floating.clear();

			if (active_icon) {
				if (shift_down) {
					floating.push_back(active_icon->create_element());
				} else {
					active_icon->set_inactive();
					active_icon = nullptr;
				}
			}
		
			current_scene->cycle_color();
			current_scene->redo_nets();
		}


	}
	if ( event.button.button == SDL_BUTTON_RIGHT) {
		class element *here = nullptr;
		float x, y;
		dragging = nullptr;

		clear_floating();
		if (active_icon)
			active_icon->set_inactive();
		active_icon = nullptr;

		x = scr_to_X(event.motion.x);
		y = scr_to_Y(event.motion.y);
		for (auto &elem : current_scene->elements) {
			if (elem->intersect(x, y)) {
				if (here == nullptr || elem->is_background() == false) {
					logger::get().debug("HERE {}", elem->class_id());
					here = elem.get();
				}
			}
		}
		active_menu = nullptr;
		auto wr = current_scene->is_wire(x, y);
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
		logger::get().debug("Right button");
	}
	if (event.button.button == SDL_BUTTON_MIDDLE) {
		middle_X = event.motion.x;
		middle_Y = event.motion.y;
		float x,y;
		x = scr_to_X(event.motion.x);
		y = scr_to_Y(event.motion.y);

		class element *here = nullptr;
		for (auto &elem : current_scene->elements) {
			if (elem->intersect(x, y) && elem->class_id() == "connector:") {
				here = elem.get();
			}
		}

		if (here) {
			logger::get().debug("Setting dragging for MM");
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
		logger::get().debug("ICON CLICK");
	if (active_icon && this_icon)
		active_icon->set_inactive();
	if (active_icon != this_icon && this_icon) {
		active_icon = this_icon;
		clear_floating();
	} else {
		if (this_icon)
			active_icon = nullptr;
	}

	if (active_icon) {
		if (floating.size() == 0) {
			auto elem = active_icon->create_element();
			if (elem) {
				elem->place(mouseX, mouseY);
				elem->start_drag(mouseX, mouseY);	
				floating.push_back(std::move(elem));
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

	if (SDL_GetTicks64() - mouse_timestamp > 1000 && active_menu == nullptr) 
		tooltip_eligable = true;
	else
		tooltip_eligable = false;

	if (timeline_bar && timeline_bar->is_active()) {
		SDL_Rect bar_rect = {main_area_rect.x, main_area_rect.h - timelinebar::BAR_HEIGHT,
		                     main_area_rect.w, timelinebar::BAR_HEIGHT};
		if (timeline_bar->handle_event(event, bar_rect))
			return false;
	}

	if (event.type == EVENT_RELOAD_ICONBAR)
		icon_bar->create_menu();

	if (event.type == EVENT_AUTOCLOCK && event.user.data1 == current_scene.get()) {
		take_undo_snapshot(current_scene.get());
		callback_autoclock(current_scene.get());
	}


	if (event.type == EVENT_REWIRE && event.user.data1 == current_scene.get()) {
		take_undo_snapshot(current_scene.get());
		current_scene->mark_layout_dirty();
		callback_reroute(current_scene.get());
		if (is_import_view_mode())
			current_scene->redo_all_multi_sink_nets();
	}

	if (event.type == EVENT_REDO_NET && event.user.data1 != nullptr && current_scene) {
		auto *seed_elem = static_cast<class element *>(event.user.data1);
		bool in_this_scene = false;
		for (const auto &el : current_scene->elements) {
			if (el.get() == seed_elem) {
				in_this_scene = true;
				break;
			}
		}
		if (in_this_scene) {
			take_undo_snapshot(current_scene.get());
			current_scene->mark_layout_dirty();
			current_scene->redo_net(seed_elem);
		}
	}

	if (event.type == EVENT_RUN_VERILOG && event.user.data1 == current_scene.get()) {
		set_dialog(std::make_unique<class synth>(main_area_rect.w + ui_area_rect.w + button_rect.w, main_area_rect.h, projectname, ""));
	}
	if (event.type == EVENT_PROGRAM_FPGA && event.user.data1 == current_scene.get()) {
		set_dialog(std::make_unique<class synth>(main_area_rect.w + ui_area_rect.w + button_rect.w, main_area_rect.h, projectname, "program"));
	}

	if (event.type == EVENT_CLOSE_DIALOG && event.user.data1 == current_scene.get()) {
		dialogbox.reset();
	}

	if (event.type == EVENT_SEQUENCER_RESET && event.user.data1 == current_scene.get()) {
		for (auto &elem : current_scene->elements)
			elem->reset_pointer();
	}
	if (event.type == EVENT_ZOOM_TO_FIT && event.user.data1 == current_scene.get()) {  /* zoom to fit the screen */
		fittoscreen = true;
		float bX1 = current_scene->sizeX, bY1 = current_scene->sizeY, bX2 = 0, bY2 = 0;
		for (auto &elem : current_scene->elements) {
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
			const int usable_h = std::max(1, main_area_rect.h -
			                                     ((timeline_bar && timeline_bar->is_active())
			                                          ? timelinebar::BAR_HEIGHT
			                                          : 0));
			from_offsetX = offsetX;
			from_offsetY = offsetY;
			from_scaleX = scaleX;
			from_scaleY = scaleY;
			animation_start = SDL_GetTicks64();
			target_scaleX = main_area_rect.w / (bX2 - bX1 + 6.0f);
			target_scaleY = usable_h / (bY2 - bY1 + 6.0f);
			target_scaleX = std::min(target_scaleX, target_scaleY);
			target_scaleY = target_scaleX;
			/* Shift past the left button bar so Column 0 inputs are never clipped behind it. */
			target_offsetX = bX1 - (static_cast<float>(button_rect.w) / std::max(1.0f, target_scaleX)) - 2.0f;
			target_offsetY = bY1 - 2.0f;

			in_animation = true;
		
			if (event.user.code != 0) {
				from_offsetX = target_offsetX;
				from_offsetY = target_offsetY;
				from_scaleX = target_scaleX;
				from_scaleY = target_scaleY;
			}
		}
	}


	for (auto &elem : current_scene->elements)
		if (elem->in_edit_mode())
			someone_in_editmode = true;
	switch (event.type) {
		case SDL_QUIT:
			logger::get().debug("QUIT");
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
							from_offsetX = offsetX;
							from_offsetY = offsetY;
							from_scaleX = scaleX;
							from_scaleY = scaleY;
							animation_start = SDL_GetTicks64();
							target_offsetX = offsetX;
							target_offsetY = offsetY;
							target_scaleX = scaleX;
							target_scaleY = scaleY;
							animation_type = 2;
							in_animation = true;
						
							fittoscreen = false;
					target_offsetY--;
					if (target_offsetY < -1)
						target_offsetY = -1;
					fittoscreen = false;
					break;
				case SDLK_DOWN:
							from_offsetX = offsetX;
							from_offsetY = offsetY;
							from_scaleX = scaleX;
							from_scaleY = scaleY;
							animation_start = SDL_GetTicks64();
							target_offsetX = offsetX;
							target_offsetY = offsetY;
							target_scaleX = scaleX;
							target_scaleY = scaleY;
							animation_type = 2;
							in_animation = true;
						
							fittoscreen = false;
							target_offsetY++;
							if (target_offsetY > current_scene->sizeY + 1)
								target_offsetY = current_scene->sizeY;
					break;
				case SDLK_LEFT:
					if (event.key.keysym.mod & KMOD_LCTRL) {
						icon_bar->previous();
					} else {
						if (!someone_in_editmode) {
							from_offsetX = offsetX;
							from_offsetY = offsetY;
							from_scaleX = scaleX;
							from_scaleY = scaleY;
							animation_start = SDL_GetTicks64();
							target_offsetX = offsetX;
							target_offsetY = offsetY;
							target_scaleX = scaleX;
							target_scaleY = scaleY;
							animation_type = 2;
							in_animation = true;
						
							target_offsetX-- ;
							if (target_offsetX < -1)
								target_offsetX = -1;
							fittoscreen = false;
						}
					}
					break;

				case SDLK_RIGHT:
					if (event.key.keysym.mod & KMOD_LCTRL) {
						icon_bar->next();
					} else {
						if (!someone_in_editmode) {
							from_offsetX = offsetX;
							from_offsetY = offsetY;
							from_scaleX = scaleX;
							from_scaleY = scaleY;
							animation_start = SDL_GetTicks64();
							target_offsetX = offsetX;
							target_offsetY = offsetY;
							target_scaleX = scaleX;
							target_scaleY = scaleY;
							animation_type = 2;
							in_animation = true;
						
							target_offsetX++ ;
							if (target_offsetX > current_scene->sizeX + 1)
								target_offsetX = current_scene->sizeX;
							fittoscreen = false;
						}
					}
					break;
				case SDLK_PLUS:
				case SDLK_KP_PLUS:
				case SDLK_EQUALS:
					if (!someone_in_editmode) {
//						scaleX *= 1.1;
//						scaleY *= 1.1;
						from_offsetX = offsetX;
						from_offsetY = offsetY;
						from_scaleX = scaleX;
						from_scaleY = scaleY;
						animation_start = SDL_GetTicks64();
						target_offsetX = offsetX;
						target_offsetY = offsetY;
						target_scaleX = scaleX * 1.1;
						target_scaleY = scaleY * 1.1;
						animation_type = 1;
						in_animation = true;
						fittoscreen = false;
					}
					break;
				case SDLK_MINUS:
				case SDLK_KP_MINUS:
					if (!someone_in_editmode) {
						from_offsetX = offsetX;
						from_offsetY = offsetY;
						from_scaleX = scaleX;
						from_scaleY = scaleY;
						animation_start = SDL_GetTicks64();
						target_offsetX = offsetX;
						target_offsetY = offsetY;
						target_scaleX = scaleX * 0.9;
						target_scaleY = scaleY * 0.9;
						in_animation = true;
						animation_type = 1;
						fittoscreen = false;
					}
					break;
				case SDLK_ESCAPE:
					clear_floating();
					active_menu = nullptr;
					if (active_icon)
						active_icon->set_inactive();
					active_icon = nullptr;
					current_scene->deselect_all();
					break;
				case SDLK_BACKSPACE:
					if (!someone_in_editmode) {
						take_undo_snapshot(current_scene.get());
						current_scene->mark_layout_dirty();
						if (auto hw = hover_wire.lock()) {
							hw->remove();
							hover_wire.reset();
						}
						current_scene->delete_selection();
						current_scene->process_delete_requests();
						current_scene->remove_orphans();
						current_scene->redo_nets();
					}
					break;
				case SDLK_SPACE:
					if (!someone_in_editmode) {
						bool rotated_any = false;
						for (auto &elem : current_scene->elements) {
							if (!elem->is_selected())
								continue;
							if (!rotated_any) {
								take_undo_snapshot(current_scene.get());
								rotated_any = true;
							}
							elem->rotate_ports();
						}
						if (rotated_any)
							current_scene->mark_layout_dirty();
					}
					break;
				case SDLK_TAB:
					if (!someone_in_editmode) {
						if (autocomplete.size() > 0) {
							take_undo_snapshot(current_scene.get());
							current_scene->mark_layout_dirty();
						}
						apply_autocomplete();
						if (dragging_wire) {
							dragging_wire.reset();
							dragging_port = nullptr;
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
						logger::get().debug("UNDO");
						swap_scene(get_undo());
						current_scene->redo_nets();
					}
					break;
				case SDLK_a:
					if (event.key.keysym.mod & KMOD_LCTRL) {
						for (auto &elem : current_scene->elements) {
							elem->select();
						}
					}
					break;
				case SDLK_c:
					if (event.key.keysym.mod & KMOD_LCTRL) {
						logger::get().debug("copy");
						json p;

						current_scene->selection_to_json(p);
						clipboard = p.dump();
						logger::get().debug("clipboard is {}", clipboard);
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

						take_undo_snapshot(current_scene.get());
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
						logger::get().debug("paste");
						json p = json::parse(clipboard);
						from_json_to_floating(p);
						for (auto &elem : floating) {
							if (elem->get_X() < x1)
								x1 = elem->get_X();
							if (elem->get_X() + elem->get_width() > x2)
								x2 = elem->get_X() + elem->get_width();
							if (elem->get_Y() < y1)
								y1 = elem->get_Y();
							if (elem->get_Y() + elem->get_height() > y2)
								y2 = elem->get_Y() + elem->get_height();
						}
						for (auto &elem : floating) {
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
				const bool is_elem_or_port_menu = (active_menu != current_scene->get_menu());
				active_menu->mouse_click(x,y);
				if (is_elem_or_port_menu)
					current_scene->mark_layout_dirty();
				active_menu = nullptr;
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
					logger::get().debug("Up");
					take_undo_snapshot(current_scene.get());
					if (!dragging->has_moved())  {
						logger::get().debug("Click");
						if (!dragging->mouse_select(x,y)) {
							dragging->select();
							if (current_scene->selected_count() == 1)
								dragging->select_single();
						}
					}
					if (dragging->stop_drag(this)) {
						current_scene->mark_layout_dirty();
						current_scene->rewire_section(dragging);
					}

					current_scene->redo_nets();
				}
				if (dragging_port && !current_scene->is_port(x, y)) {
					std::shared_ptr<class wire> wr;

					take_undo_snapshot(current_scene.get());
					current_scene->mark_layout_dirty();

					wr = current_scene->is_wire(x, y);
					if (wr) {
						class port *port;
						auto conn = std::make_unique<connector>(x, y);
						class element *_element = conn.get();
						current_scene->add_element(std::move(conn));

						auto wr2 = wr->split();
						port = current_scene->is_port(x,y);
						if (!port)
							logger::get().debug("No port found");
						else
							logger::get().debug("Port {} found", port->name);
						wr->add_port(port);
						wr2->add_port(port);
						port->add_wire(wr);
						port->add_wire(wr2);
						wr->reseat();
						wr2->reseat();
						wr->route(current_scene.get());
						wr2->route(current_scene.get());
						_element->reroute_all_wires();
						logger::get().debug("new split code");
					} else {
						logger::get().debug("not new split code");
						current_scene->add_element(std::make_unique<connector>(x, y));
					}

					current_scene->cycle_color();
					current_scene->redo_nets();
				}
				if (dragging_port && current_scene->is_port(x, y)) {
					class port *port2 = current_scene->is_port(x, y);

					if (dragging_port != port2) {
						take_undo_snapshot(current_scene.get());
						current_scene->mark_layout_dirty();
						zap_autocomplete();
						dragging_port->add_wire(dragging_wire);
						port2->add_wire(dragging_wire);
						dragging_wire->reseat();
						dragging_wire->route(current_scene.get());
						dragging_wire.reset();
						logger::get().debug("Connection made");

						current_scene->redo_nets();
						create_autocomplete_from_wire(dragging_port, port2);
					}
				} 
				dragging = nullptr;
				dragging_port = nullptr;
				left_mouse_down = false;
				dragging_wire.reset();
			}
			if (event.button.button == SDL_BUTTON_MIDDLE) {
				float x, y;
				x = scr_to_X(event.motion.x);
				y = scr_to_Y(event.motion.y);
				middle_X = event.motion.x;
				middle_Y = event.motion.y;
				if (dragging) {
					logger::get().debug("Up");
					take_undo_snapshot(current_scene.get());
					if (!dragging->has_moved())  {
						logger::get().debug("Click");
						if (!dragging->mouse_select(x,y)) {
							dragging->select();
							if (current_scene->selected_count() == 1)
								dragging->select_single();
						}
					}
					if (dragging->stop_drag(this)) {
						current_scene->mark_layout_dirty();
						current_scene->rewire_section(dragging);
					}
					dragging = nullptr;

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
				active_menu = nullptr;

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
				for (auto &elem : current_scene->elements) {
					if (elem->get_X() >= X1 && elem->get_X()+elem->get_width() <= X2 && elem->get_Y() >= Y1 && elem->get_Y()+elem->get_height() <= Y2)
						elem->select();
				}
			}

			if (auto hw = hover_wire.lock())
				hw->deselect();
			hover_wire = current_scene->is_wire(x, y);
			if (auto hw = hover_wire.lock())
				hw->select();

			if (dragging) {
				dragging->update_drag(this, current_scene.get(),
						scr_to_X(event.motion.x),
						scr_to_Y(event.motion.y));
			}
			for (auto &flt : floating) {
				flt->update_drag(this, current_scene.get(),
						scr_to_X(event.motion.x),
						scr_to_Y(event.motion.y));
			}
			if (dragging_wire) {
				bool banned = false;
				for (auto &elem : current_scene->elements) {
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
						callback_fit_to_screen2(current_scene.get());
					if (dialogbox)
						dialogbox->update_screen_size(event.window.data1, event.window.data2);
					break;
				case SDL_WINDOWEVENT_CLOSE:   /* user clicked the 'x' */
					SDL_HideWindow(window.get());
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

	for (auto &elem : current_scene->elements)
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
	if (!window_shown) {
		return;
	}
		
	/* first, draw the lighter gray background */

	SDL_SetRenderDrawColor(renderer.get(), R(COLOR_BACKGROUND_GRID),
			G(COLOR_BACKGROUND_GRID),
			B(COLOR_BACKGROUND_GRID),
			Alpha(COLOR_BACKGROUND_GRID));
	SDL_RenderClear(renderer.get());
	basecanvas::draw();
	icon_bar->draw();
	SDL_RenderSetClipRect(renderer.get(), &main_area_rect);

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
		wiregrid grid(current_scene->sizeX, current_scene->sizeY);
		current_scene->fill_grid(&grid);

		for (y = 0; y < current_scene->sizeY ; y++)
			for (x = 0; x < current_scene->sizeX ; x++) {
				float c = grid.get_soft_cost(x, y);
				if (c > 0.01) {
					draw_box(x + 0.1, y + 0.1, x + 0.9, y + 0.9, COLOR_ELEMENT_GHOST, 64 * c);
				}
				if (grid.is_blocked(x,y)) {
					draw_box(x + 0.1, y + 0.1, x + 0.9, y + 0.9, COLOR_ELEMENT_DND);
				} 
			}
	}

	/* draw the elements */
	for (auto const &elem : current_scene->elements) {
		if (elem.get() != dragging)
			elem->draw_early(this, DRAW_NORMAL);
		else
			elem->draw_early(this, DRAW_ORIGIN);
	}

	for (auto const &elem : current_scene->elements) {
		if (elem.get() != dragging)
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
	for (auto &flt : floating) {
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

	for (auto const &elem : current_scene->elements) {
		if (elem.get() != dragging)
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

		draw_image(area_select_texture.get(), X1, Y1, X2-X1, Y2-Y1, 64);
	}

	SDL_RenderSetClipRect(renderer.get(), nullptr);
	if (fps > 0)
		button_bar->set_fps(fps);
	button_bar->draw_at(this, button_rect.w, button_rect.h);

	if (timeline_bar && timeline_bar->is_active()) {
		SDL_Rect bar_rect = {main_area_rect.x, main_area_rect.h - timelinebar::BAR_HEIGHT,
		                     main_area_rect.w, timelinebar::BAR_HEIGHT};
		timeline_bar->draw(this, bar_rect);
	}

	draw_hover_inspector();

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
		SDL_RenderSetClipRect(renderer.get(), nullptr);
		active_menu->draw_at(this);
	}

	if (dialogbox)
		dialogbox->draw(this);

}

static float scale = -1;

static void fill_scale(void)
{
	if (scale > 0)
		return;
	float dpi;
	SDL_GetDisplayDPI(0, &dpi, nullptr, nullptr);
	if (dpi < 90)
		dpi = 96;
	scale = 1.3 * 96.0 / dpi;
}


void canvas::draw_tooltip(float X, float Y, std::string_view tooltip)
{
	SDL_Texture *text;
	fill_scale();

	text = text_to_texture(tooltip);
	if (!text)
		return;

	SDL_Rect rect;
	SDL_Point size;
	int x, y;

	SDL_QueryTexture(text, nullptr, nullptr, &size.x, &size.y);

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

	SDL_RenderSetClipRect(renderer.get(), nullptr);	
	SDL_SetRenderDrawColor(renderer.get(), 45, 45, 45, 220);
	SDL_RenderFillRect(renderer.get(), &rect);
	SDL_RenderCopy(renderer.get(), text, nullptr, &rect);
}

void canvas::draw_hover_inspector(void)
{
	if (!current_scene || active_menu || dialogbox || dragging || in_area_select)
		return;

	const int scr_x = X_to_scr(mouseX);
	const int scr_y = Y_to_scr(mouseY);
	const int bottom_limit = (timeline_bar && timeline_bar->is_active())
	                             ? (main_area_rect.h - timelinebar::BAR_HEIGHT)
	                             : main_area_rect.h;
	if (scr_x < main_area_rect.x || scr_x >= main_area_rect.x + main_area_rect.w ||
	    scr_y < main_area_rect.y || scr_y >= bottom_limit) {
		return;
	}

	/* Priority: hovered non-connector element first, then hovered named wire. */
	element *hovered_elem = nullptr;
	for (auto &elem : current_scene->elements) {
		if (!elem || elem->is_background() || elem->class_id() == "connector:")
			continue;
		if (elem->intersect(mouseX, mouseY)) {
			hovered_elem = elem.get();
			break;
		}
	}

	std::string top_text;
	std::string verilog_text;

	if (hovered_elem) {
		std::string label = hovered_elem->get_name();
		if (label.empty())
			label = hovered_elem->class_id();
		top_text = label;
		if (!hovered_elem->get_src_location().empty())
			top_text += "   [" + hovered_elem->get_src_location() + "]";
		verilog_text = hovered_elem->get_src_verilog();
	} else if (auto hw = hover_wire.lock()) {
		const bool is_named = !hw->get_vcd_name().empty() ||
		                      (!hw->name.empty() && !hw->name.starts_with("wire_"));
		if (is_named) {
			const std::string wlabel = !hw->name.empty() ? hw->name : hw->get_vcd_name();
			const int w = hw->get_width();
			if (w > 1) {
				top_text = std::format("{} [{}:0]", wlabel, w - 1);
				if (hw->value.valid) {
					top_text += std::format(" = 0x{:X}", hw->value.intval);
					const std::string enum_lbl = hw->get_label(hw->value.intval);
					if (!enum_lbl.empty())
						top_text += " (" + enum_lbl + ")";
				}
			} else {
				top_text = wlabel;
				if (hw->value.valid)
					top_text += hw->value.boolval ? " = 1" : " = 0";
			}
			if (!hw->get_src_location().empty())
				top_text += "   [" + hw->get_src_location() + "]";
			verilog_text = hw->get_src_verilog();
		}
	}

	if (top_text.empty() && verilog_text.empty())
		return;

	fill_scale();
	SDL_RenderSetClipRect(renderer.get(), nullptr);

	/* 1. Top single-line name + file:line banner. */
	if (!top_text.empty()) {
		SDL_Texture *tex = text_to_texture(top_text);
		if (tex) {
			int tw = 0, th = 0;
			SDL_QueryTexture(tex, nullptr, nullptr, &tw, &th);
			const int draw_w = std::max(1, static_cast<int>(tw / scale));
			const int draw_h = std::max(1, static_cast<int>(th / scale));
			constexpr int pad_x = 12;
			constexpr int pad_y = 6;

			SDL_Rect bg = {main_area_rect.x, main_area_rect.y,
			               main_area_rect.w, draw_h + 2 * pad_y};
			SDL_SetRenderDrawColor(renderer.get(), 18, 20, 24, 210);
			SDL_RenderFillRect(renderer.get(), &bg);
			SDL_SetRenderDrawColor(renderer.get(), 90, 100, 115, 220);
			SDL_RenderDrawLine(renderer.get(), bg.x, bg.y + bg.h - 1,
			                   bg.x + bg.w - 1, bg.y + bg.h - 1);

			SDL_Rect clip = {bg.x + pad_x, bg.y + pad_y,
			                 std::max(1, bg.w - 2 * pad_x), draw_h};
			SDL_RenderSetClipRect(renderer.get(), &clip);
			SDL_Rect dst = {bg.x + pad_x, bg.y + pad_y, draw_w, draw_h};
			SDL_RenderCopy(renderer.get(), tex, nullptr, &dst);
			SDL_RenderSetClipRect(renderer.get(), nullptr);
		}
	}

	/* 2. Bottom-right multi-line Verilog snippet box (just above timeline bar, right-aligned to element bar). */
	if (!verilog_text.empty()) {
		constexpr size_t MAX_LINES = 18;
		std::vector<std::string> lines;
		size_t pos = 0;
		bool truncated = false;
		while (pos <= verilog_text.size()) {
			size_t nl = verilog_text.find('\n', pos);
			std::string raw = (nl == std::string::npos)
			                      ? verilog_text.substr(pos)
			                      : verilog_text.substr(pos, nl - pos);
			if (!raw.empty() && raw.back() == '\r')
				raw.pop_back();

			/* Expand tabs to 4 spaces so alignment is readable. */
			std::string expanded;
			expanded.reserve(raw.size());
			for (char ch : raw) {
				if (ch == '\t')
					expanded.append("    ");
				else
					expanded.push_back(ch);
			}
			if (lines.size() >= MAX_LINES) {
				truncated = true;
				break;
			}
			lines.push_back(std::move(expanded));
			if (nl == std::string::npos)
				break;
			pos = nl + 1;
		}
		if (truncated && !lines.empty())
			lines.back() = "...";

		const float code_scale = scale * 1.15f;
		constexpr int pad_x = 12;
		constexpr int pad_y = 8;
		constexpr int line_gap = 2;
		constexpr int margin = 8;

		struct line_tex {
			SDL_Texture *tex = nullptr;
			int w = 0;
			int h = 0;
		};
		std::vector<line_tex> rendered;
		rendered.reserve(lines.size());

		int max_w = 0;
		int default_h = 16;
		int total_h = 0;
		for (const auto &ln : lines) {
			line_tex lt;
			if (!ln.empty()) {
				lt.tex = text_to_texture(ln);
				if (lt.tex) {
					int tw = 0, th = 0;
					SDL_QueryTexture(lt.tex, nullptr, nullptr, &tw, &th);
					lt.w = std::max(1, static_cast<int>(tw / code_scale));
					lt.h = std::max(1, static_cast<int>(th / code_scale));
					default_h = lt.h;
					max_w = std::max(max_w, lt.w);
				}
			}
			if (lt.h == 0)
				lt.h = default_h;
			total_h += lt.h + line_gap;
			rendered.push_back(lt);
		}
		if (!rendered.empty())
			total_h -= line_gap;

		if (max_w > 0 && total_h > 0) {
			const int max_box_w = std::max(100, main_area_rect.w - 2 * margin);
			const int box_w = std::min(max_w + 2 * pad_x, max_box_w);
			const int box_h = total_h + 2 * pad_y;
			const int box_x = main_area_rect.x + main_area_rect.w - margin - box_w;
			const int box_y = std::max(main_area_rect.y + 40, bottom_limit - margin - box_h);

			SDL_Rect bg = {box_x, box_y, box_w, box_h};
			SDL_SetRenderDrawColor(renderer.get(), 18, 20, 24, 215);
			SDL_RenderFillRect(renderer.get(), &bg);
			SDL_SetRenderDrawColor(renderer.get(), 90, 100, 115, 220);
			SDL_RenderDrawRect(renderer.get(), &bg);

			SDL_Rect clip = {bg.x + pad_x, bg.y + pad_y,
			                 std::max(1, bg.w - 2 * pad_x),
			                 std::max(1, bg.h - 2 * pad_y)};
			SDL_RenderSetClipRect(renderer.get(), &clip);

			int cur_y = bg.y + pad_y;
			for (const auto &lt : rendered) {
				if (lt.tex) {
					SDL_Rect dst = {bg.x + pad_x, cur_y, lt.w, lt.h};
					SDL_RenderCopy(renderer.get(), lt.tex, nullptr, &dst);
				}
				cur_y += lt.h + line_gap;
			}
			SDL_RenderSetClipRect(renderer.get(), nullptr);
		}
	}
}



void canvas::to_json(json& j)
{
	j["offsetX"] = offsetX;
	j["offsetY"] = offsetY;
	j["scaleX"] = scaleX;
	j["scaleY"] = scaleY;
}

std::unique_ptr<class scene> canvas::swap_scene(std::unique_ptr<class scene> scene)
{
	if (!scene)
		return nullptr;
	auto tmp = std::move(current_scene);
	current_scene = std::move(scene);
	return tmp;
}

#define UNDO_DEPTH 16

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


std::unique_ptr<class scene> canvas::get_undo(void)
{
	std::string s;
	json J;

	clear_wire_factory();

	if (undo_list.size() == 0)
		return nullptr;

	auto scene = std::make_unique<class scene>(current_scene->get_name());
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
		auto element = element_from_class_id(cid);
		element->from_json(p);
		floating.push_back(std::move(element));
	}
	clear_wire_factory();
	wire_factory_force_new_name = saved;

}


void canvas::update_window_title(void)
{
	std::string s;

	s = "GridCad " + current_scene->get_full_name();
	SDL_SetWindowTitle(window.get(), s.c_str());
}

float canvas::distance_from_mouse(float X, float Y)
{
	return sqrtf( (X-mouseX)*(X-mouseX) + (Y-mouseY)*(Y-mouseY));
}

void canvas::clear_floating(void)
{
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
	for (auto &autoc : autocomplete) 
		if (autoc->tempwire) {
			autoc->tempwire->draw(this, COLOR_ELEMENT_GHOST);
		}
}

void canvas::create_autocomplete_from_wire(class port *first, class port *second)
{
	class element *frome = nullptr, *toe = nullptr;
	int from_port_index = -100;
	int to_port_index = -100;

	zap_autocomplete();

	logger::get().debug("Creating multiwire autocomplete");	


	for (auto &elem : current_scene->elements) {
		if (elem->has_port(first) >= 0) {
			from_port_index = elem->has_port(first);
			frome = elem.get();
		}
		if (elem->has_port(second) >= 0) {
			to_port_index = elem->has_port(second);
			toe = elem.get();
		}
	}

	if (from_port_index < 0 || to_port_index < 0) {
		logger::get().debug("Couldn't find both ports");
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


		auto autoc = std::make_unique<class autocomplete_element>();
		autoc->from = frome;
		autoc->to = toe;
		autoc->from_port = candidate_from;
		autoc->to_port = candidate_to;

		if (candidate_to->direction == PORT_IN)
			autoc->tempwire = std::make_shared<class wire>(autoc->to_port->screenX, autoc->to_port->screenY, autoc->from_port->screenX, autoc->from_port->screenY, 0);
		else
			autoc->tempwire = std::make_shared<class wire>(autoc->from_port->screenX, autoc->from_port->screenY, autoc->to_port->screenX, autoc->to_port->screenY, 0);
		autocomplete.push_back(std::move(autoc));

		from_port_index++;
		to_port_index++;
	};

	logger::get().debug("Creating autocomplete of size {}", autocomplete.size());


}

void canvas::apply_autocomplete(void)
{
	class port *p1 = nullptr, *p2 = nullptr;
	for (auto &autoc : autocomplete) {
		auto wire = std::move(autoc->tempwire);

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
	class port *target = nullptr;
	class element *targete = nullptr;
	class element *sourcee = nullptr;
	double target_dist = 500*500*1000;

	zap_autocomplete();

	for (auto &elem : current_scene->elements) {
		unsigned int i = 0;
		class port *p = nullptr;
		do {
			p = elem->port_at(i);
			if (!p)
				break;
			i++;
			if (p == origin)
				sourcee = elem.get();
		} while(p);
	}

	for (auto &elem : current_scene->elements) {
		unsigned int i = 0;
		class port *p = nullptr;
		do {	
			float dist;
			p = elem->port_at(i);
			if (!p)
				break;
			i++;
			if (p == origin)
				sourcee = elem.get();
			if (p->has_wires())
				continue;
			if (!direction_compatible(origin, p))
				continue;
			if (!width_compatible(origin, p))
				continue;

			if (sourcee == elem.get())
				continue;

			dist = distsq(origin->screenX, origin->screenY, p->screenX, p->screenY);

			/* 50% discount of the ports have the same name */
			if (origin->name == p->name)
				dist = dist/2;

			if (dist < target_dist) {
				target_dist = dist;
				target = p;
				targete = elem.get();
			}

		} while (p);
	}
	if (target == nullptr || sourcee == nullptr || targete == nullptr)
		return;
	auto autoc = std::make_unique<class autocomplete_element>();
	autoc->from = sourcee;
	autoc->to = targete;
	autoc->from_port = origin;
	autoc->to_port = target;
	if (target->direction == PORT_IN)
		autoc->tempwire = std::make_shared<class wire>(autoc->to_port->screenX, autoc->to_port->screenY, autoc->from_port->screenX, autoc->from_port->screenY, 0);
	else
		autoc->tempwire = std::make_shared<class wire>(autoc->from_port->screenX, autoc->from_port->screenY, autoc->to_port->screenX, autoc->to_port->screenY, 0);
	autocomplete.push_back(std::move(autoc));


}
