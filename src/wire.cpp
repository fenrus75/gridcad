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
#include "scene.h"
#include "wirepath.h"
#include "wire.h"
#include "port.h"
#include "net.h"
#include "model_clock.h"

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <format>

#include <sys/time.h>
#include <map>

extern bool wire_debug_mode ;

wire::wire(int x1, int y1, int x2, int y2, int _color)
{
	X1 = x1;
	Y1 = y1;
	X2 = x2;
	Y2 = y2;
	color = _color;

	points.reset();
	value = {};
	name = generate_wire_name();;

}

wire::~wire(void)
{
	if (net) {
		net->remove_wire(this);
		net = nullptr;
	}
}

void wire::set_new_name(void)
{
	name = generate_wire_name();

}



float dist(float x1, float y1, float x2, float y2)
{
	float dx, dy,d;
	dx = x2-x1;
	dy = y2-y1;
	d = sqrtf(dx*dx + dy*dy);
	return d;
}

float distsq(float x1, float y1, float x2, float y2)
{
	float dx, dy,d;
	dx = x2-x1;
	dy = y2-y1;
	d = dx*dx + dy*dy;
	return d;
}

int calc_angle(int x1, int y1, int x2, int y2)
{
	int dx, dy;

	dx = x2 - x1;
	if (dx > 1) dx = 1;
	if (dx < -1) dx = -1;
	dy = y2 - y1;
	if (dy > 1) dy = 1;
	if (dy < -1) dy = -1;


	if (dx ==  1 && dy ==  0) return 180;
	if (dx ==  1 && dy ==  1) return 45;
	if (dx ==  0 && dy == -1) return 270;
	if (dx ==  1 && dy == -1) return 135;
	if (dx == -1 && dy ==  0) return 0;
	if (dx == -1 && dy == -1) return 225;
	if (dx ==  0 && dy ==  1) return 90;
	if (dx == -1 && dy ==  1) return 315;


	return 0;
}

void draw_snake_line(class basecanvas *canvas, float x1, float y1, float x2, float y2, int color, float *dist_to_next, int *box_parity, struct value *value, float spacing, class wire *wire, float pulse, std::string_view enum_label)
{
	const float d = dist(x1, y1, x2, y2);
	if (d <= 1e-6f || spacing <= 1e-6f)
		return;

	if (!canvas->segment_visible(x1, y1, x2, y2)) {
		/* Advance continuous phase in O(1) so boxes entering the viewport stay seamlessly stitched. */
		float rem = *dist_to_next - d;
		if (rem <= 0.0f) {
			const int crossed = static_cast<int>(std::floor(-rem / spacing)) + 1;
			rem += static_cast<float>(crossed) * spacing;
			if (crossed & 1)
				(*box_parity) ^= 1;
		}
		*dist_to_next = rem;
		return;
	}

	const bool thick = (value->type == VALUE_TYPE_INT);
	if (thick)
		canvas->draw_thick_line(x1, y1, x2, y2, color);
	else
		canvas->draw_line(x1, y1, x2, y2, color);

	if (!canvas->canvas_has_focus())
		return;

	if (canvas->scale_to_X(1) > 1/20.0)
		return;

	const int ang = calc_angle(roundf(x1), roundf(y1), roundf(x2), roundf(y2));
	const float dx = x2 - x1;
	const float dy = y2 - y1;
	float pos = *dist_to_next;

	while (pos <= d) {
		const float t = pos / d;
		const float bx = x1 + dx * t;
		const float by = y1 + dy * t;
		const bool show_enum = (!enum_label.empty() && (((*box_parity) & 1) == 0));
		(*box_parity) ^= 1;

		if (canvas->segment_visible(bx, by, bx, by)) {
			if (value->type == VALUE_TYPE_INT || wire_debug_mode || wire->get_is_z()) {
				float cursormag = 2.5f - canvas->distance_from_mouse(bx, by) / 2.0f;
				cursormag = std::clamp(cursormag, 1.0f, 2.0f);
				const bool qual_invalid = (!value->qualifier_valid && !wire->get_is_z() && !wire_debug_mode);
				const float eff_pulse = qual_invalid ? 0.0f : pulse;
				const float pulse_mag = 1.0f + 0.35f * eff_pulse;
				const float size = 0.24f * cursormag * pulse_mag;
				const float size2 = 0.20f * cursormag * pulse_mag;
				if (qual_invalid) {
					/* Qualified-invalid signal: muted grey box without white border or text. */
					canvas->draw_box_rgba(bx - size, by - size, bx + size, by + size, 100, 100, 100, 220);
					canvas->draw_box_rgba(bx - size2, by - size2, bx + size2, by + size2, 68, 68, 68, 220);
				} else {
					std::string s;
					bool has_enum_label = false;
					if (wire->get_is_z())
						s = "Z";
					else if (wire_debug_mode)
						s = std::format("{}", wire->get_distance_from_outport());
					else if (show_enum) {
						s = std::string(enum_label);
						has_enum_label = true;
					} else {
						s = std::format("{}", value->intval);
					}
					const float half_w = has_enum_label ? std::max(size, size * 0.45f * static_cast<float>(s.size())) : size;
					const float half_w2 = half_w - (size - size2);
					if (eff_pulse > 0.001f) {
						const Uint8 br = static_cast<Uint8>(R(COLOR_WIRE_MOTION) + (255 - R(COLOR_WIRE_MOTION)) * eff_pulse);
						const Uint8 bg = static_cast<Uint8>(G(COLOR_WIRE_MOTION) + (225 - G(COLOR_WIRE_MOTION)) * eff_pulse);
						const Uint8 bb = static_cast<Uint8>(B(COLOR_WIRE_MOTION) + (80 - B(COLOR_WIRE_MOTION)) * eff_pulse);
						canvas->draw_box_rgba(bx - half_w, by - size, bx + half_w, by + size, br, bg, bb, 255);
					} else {
						canvas->draw_box(bx - half_w, by - size, bx + half_w, by + size, COLOR_WIRE_MOTION);
					}
					canvas->draw_box(bx - half_w2, by - size2, bx + half_w2, by + size2, COLOR_BACKGROUND_MAIN);
					canvas->draw_text(s, bx + 0.05f - half_w, by + 0.05f - size, 2.0f * half_w - 0.1f, 2.0f * size - 0.1f);
				}
			} else if (value->is_clock) {
				const float size = 0.24f;
				float f = std::fmod(2.0f * pos / spacing, 1.0f);
				f = (1.0f - f) * (450.0f - 25.0f);
				if (!clock_running)
					f = 0.0f;
				canvas->draw_box(bx - size, by - size, bx + size, by + size, COLOR_WIRE_MOTION);
				canvas->draw_image_fragment("assets/clock_wave.png", bx - size, by - size, 2.0f * size, 2.0f * size, 25 + f, 0, 450 - 25, 189, ang);
			} else {
				float r = thick ? 0.22f : 0.18f;
				r *= (1.0f + 0.45f * pulse);
				canvas->draw_image(value_color_texture(value), bx - r, by - r, 2.0f * r, 2.0f * r);
			}
		}
		pos += spacing;
	}
	*dist_to_next = pos - d;
}

void wire::draw(class basecanvas *canvas, int _color)
{
	float prevX, prevY;
	bool first = true;

	assert(!dead);

	/* Each wire is referenced by both endpoint ports; skip the second draw within the same frame. */
	const uint64_t fid = canvas->current_frame_id();
	if (fid != 0 && _color != COLOR_ELEMENT_GHOST) {
		if (last_drawn_frame == fid && last_drawn_canvas == canvas)
			return;
		last_drawn_frame = fid;
		last_drawn_canvas = canvas;
	}

	if (!points)
		route(canvas->get_scene());
		
	calculate_drawpoints();	

	if (!drawpoints) {
		return;
	}

	/* Continuous floating-point phase (5.0 grid units/sec) avoids 60 Hz integer beat stutter.
	 * Increase value-badge density across the entire net whenever any segment on it is hovered. */
	bool net_selected = selected;
	if (!net_selected && net) {
		for (const class wire *w : net->get_wires()) {
			if (w && w->is_selected()) {
				net_selected = true;
				break;
			}
		}
	}
	const float spacing = net_selected ? (20.0f / 12.0f) : 5.0f;
	double phase_boxes = (static_cast<double>(SDL_GetTicks()) * 0.005) / static_cast<double>(spacing);
	if (value.is_clock && !clock_running)
		phase_boxes = (static_cast<double>(global_clock.arrayval) * (4.0 / 12.0)) / static_cast<double>(spacing);
	const double whole = std::floor(phase_boxes);
	const double frac = phase_boxes - whole;
	float dist_to_next = static_cast<float>((1.0 - frac) * static_cast<double>(spacing));
	int box_parity = static_cast<int>(static_cast<uint64_t>(whole) & 1ULL);

	const std::string enum_label =
		(value.type == VALUE_TYPE_INT && !wire_debug_mode && !is_z) ? get_label(value.intval) : "";

	const float pulse = (_color != COLOR_ELEMENT_GHOST) ? change_pulse() : 0.0f;
	const bool is_crit = (show_critical_path && timing.critical_path && _color != COLOR_ELEMENT_GHOST);
	const int shadow_color = (pulse > 0.001f && _color != COLOR_VALUE_RED) ? wire_to_color(color) : _color;

	for (auto point: *drawpoints) {
		if (first) {
			prevX = point.X;
			prevY = point.Y;
			first = false;
			continue;
		}
		if (canvas->segment_visible(prevX + 0.5f, prevY + 0.5f, point.X + 0.5f, point.Y + 0.5f))
			canvas->draw_shadow_Line(prevX + 0.5, prevY + 0.5, point.X + 0.5, point.Y + 0.5, shadow_color, pulse, is_crit);
		prevX = point.X;
		prevY = point.Y;
	}
	first =true;

	/* only draw the snake line on windows that have the focus -- it's resource intensive */
	for (auto point: *drawpoints) {
		if (first) {
			prevX = point.X;
			prevY = point.Y;
			first = false;
			continue;
		}
		
		float tX, tY;
		tX = prevX + 0.25 * (point.X - prevX) + 0.5;
		tY = prevY + 0.25 * (point.Y - prevY) + 0.5;
		if (wire_debug_mode)
			canvas->draw_circle(tX,tY, 0.4, COLOR_VALUE_RED);
		
		if (_color != COLOR_ELEMENT_GHOST)
			draw_snake_line(canvas, prevX + 0.5, prevY + 0.5, point.X + 0.5, point.Y + 0.5, wire_to_color(color), &dist_to_next, &box_parity, &value, spacing, this, pulse, enum_label);
		else
			draw_snake_line(canvas, prevX + 0.5, prevY + 0.5, point.X + 0.5, point.Y + 0.5, COLOR_ELEMENT_GHOST, &dist_to_next, &box_parity, &value, spacing, this, 0.0f, enum_label);
		prevX = point.X;
		prevY = point.Y;
	}
}

void wire::move_target(int X, int Y)
{
	if (X2 == X && Y2 == Y)
		return;
	X2 = X;
	Y2 = Y;
	if (points) {
		oldpoints = std::move(points);
	}
	points.reset();
}

void wire::clear_route(void)
{
	if (!points)
		return;
	oldpoints = std::move(points);
}

void wire::set_routed_points(std::unique_ptr<std::vector<struct waypoint>> pts)
{
	points = std::move(pts);
	oldpoints.reset();
	in_animation = false;
	if (points && !points->empty()) {
		X1 = static_cast<int>(std::round(points->front().X));
		Y1 = static_cast<int>(std::round(points->front().Y));
		X2 = static_cast<int>(std::round(points->back().X));
		Y2 = static_cast<int>(std::round(points->back().Y));
		check_reverse();
		calculate_drawpoints();
	}
}

static bool is_rail_wire(const class wire *w)
{
	return w->value.is_clock || w->name == "clk" || w->name == "rst" || w->name == "rst_n";
}

void wire::route(class scene *scene)
{
	being_routed = true;
	
	if (points) {
		oldpoints = std::move(points);
	}
		

//	printf("Routing wire\n");

	wiregrid grid(scene->sizeX, scene->sizeY);

	const bool rail = is_rail_wire(this);
	grid.is_clock = rail;

	scene->fill_grid(&grid);

	points = grid.path_walk(X1, Y1, X2, Y2, rail);

	bool want_reverse = false;

	for (auto port : ports) {
		if (floorf(port->screenX) == X1 && floorf(port->screenY) == Y1 && port->direction == PORT_OUT) {
			want_reverse = true;
		}
		if (floorf(port->screenX) == X2 && floorf(port->screenY) == Y2 && port->direction == PORT_IN) {
			want_reverse = true;
		}
	}

	if (ports.size() > 1) {
		if (floorf(ports[0]->screenX) == X1 && floorf(ports[0]->screenY) == Y1 && ports[0]->get_distance_from_outport() < ports[1]->get_distance_from_outport()) 
			want_reverse = true;
	}
	if (want_reverse)  {
		float tX = X1, tY = Y1;
		X1 = X2;
		Y1 = Y2;
		X2 = tX;
		Y2 = tY;
		std::reverse(points->begin(), points->end());
		want_reverse = false;
	}
	is_reversed = want_reverse;
	being_routed = false;
//	reseat();
	
	points_timestamp = SDL_GetTicks();
	in_animation = true;
}

void wire::check_reverse(void)
{
	being_routed = true;
	bool want_reverse = false;
	reseat();

	if (!points)
		return;
		
	if (ports.size() < 2)
		return;

#if 0		
	printf("Checking reverse. ports (%5.2f, %5.2f @ %i)  (%5.2f, %5.2f @ %i)   wire start (%5.2f %5.2f) @ (%i, %i)\n",
			ports[0]->screenX, ports[0]->screenY, ports[0]->get_distance_from_outport(), 
			ports[1]->screenX, ports[1]->screenY, ports[1]->get_distance_from_outport(),
			(*points)[0].X, (*points)[0].Y, X1, Y1);
#endif

	if (ports[0]->get_distance_from_outport() < ports[1]->get_distance_from_outport()) 
		want_reverse = true;
	
	if (want_reverse && points->size()>1) {
		float tX = X1, tY = Y1;
		X1 = X2;
		Y1 = Y2;
		X2 = tX;
		Y2 = tY;
		std::reverse(points->begin(), points->end());
		in_animation = true;
//		printf("reversing\n");
		want_reverse = false;
		reseat();
#if 0
		printf("	post Checking reverse. ports (%5.2f, %5.2f @ %i)  (%5.2f, %5.2f @ %i)   wire start (%5.2f %5.2f) @ (%i, %i)\n",
			ports[0]->screenX, ports[0]->screenY, ports[0]->get_distance_from_outport(), 
			ports[1]->screenX, ports[1]->screenY, ports[1]->get_distance_from_outport(),
			(*points)[0].X, (*points)[0].Y, X1, Y1);
#endif
	}
	being_routed = false;
	is_reversed = false;
}

void wire::add_port(class port *port)
{
	for (auto _port : ports) {
		if (port == _port)
			return;
	}
	ports.push_back(port);
	if (port->direction != PORT_IN) {
		update_value_net(&port->value, DEFAULT_TTL);
	}
}

void wire::del_port(class port *port)
{
	for (unsigned int i = 0; i< ports.size(); i++) {
		auto _port = ports[i];
		if (port == _port) {
			ports.erase(ports.begin() + i);
			if (net && port) {
				bool still_on_net = false;
				for (const auto &pw : port->get_wires()) {
					if (pw && pw.get() != this && pw->get_net() == net) {
						still_on_net = true;
						break;
					}
				}
				if (!still_on_net)
					net->remove_port(port);
			}
			return;
		}
	}
}
void wire::reseat(void)
{
	if (points && ports.size() > 1) {
		if ((*points)[0].X == ports[1]->screenX && (*points)[0].Y == ports[1]->screenY) {
//			printf("flipping ports\n");
			std::reverse(ports.begin(), ports.end());
		}
	}
	if (ports.size() > 0) {
		X1 = ports[0]->screenX;
		Y1 = ports[0]->screenY;
	}
	if (ports.size() > 1) {
		X2 = ports[1]->screenX;
		Y2 = ports[1]->screenY;
	}
}
#if 0
void wire::update_value(struct value *newvalue, int ttl)
{
	if (ttl <= 0)
		return;
	if (!newvalue->valid)
		return;
	if (value.valid && value == *newvalue) {
		/* no change -- early exit to kill oscillations */
		return;
	}
	value = *newvalue;
	if (ttl < 10) {
		logger::get().warn("wire ttl exceeded {}", ttl);
		newvalue->is_error = true;
	}

	/* now to notify the ports we're connected to */
	for (auto port: ports) {
		if (port->direction != PORT_OUT)
			port->update_value(&value, ttl -1);
	}
	notify(ttl - 1);
}
#endif

void wire::record_value_change(const struct value &v)
{
	if (value.valid && v.valid && v.qualifier_valid && !v.is_clock && value != v)
		last_changed_ms = SDL_GetTicks();
}

float wire::change_pulse(void) const
{
	if (last_changed_ms == 0)
		return 0.0f;
	/* Manual single-stepping gets 2x duration (100ms attack + 1100ms decay = 1200ms) so the user
	 * has time to scan changes; automatic playback uses 50ms + 550ms = 600ms. */
	const uint64_t attack_ms = clock_running ? 50 : 100;
	const uint64_t decay_ms = clock_running ? 550 : 1100;
	const uint64_t total_ms = attack_ms + decay_ms;
	const uint64_t now = SDL_GetTicks();
	if (now < last_changed_ms)
		return 0.0f;
	const uint64_t elapsed = now - last_changed_ms;
	if (elapsed >= total_ms)
		return 0.0f;
	if (elapsed < attack_ms)
		return static_cast<float>(elapsed) / static_cast<float>(attack_ms);
	const float t = static_cast<float>(elapsed - attack_ms) / static_cast<float>(decay_ms);
	return std::max(0.0f, 1.0f - std::sin(t * 1.57079632679f));
}

void wire::update_value_final(struct value *newvalue, int ttl)
{
	if (vcd_locked)
		return;
	record_value_change(*newvalue);
	value = *newvalue;
	notify(ttl - 1);
}

void wire::update_value_net(struct value *newvalue, int ttl)
{
	if (vcd_locked)
		return;
	if (ttl <= 0)
		return;

	if (!net)
		return;

	net->update_value(newvalue, ttl - 1);
}

void wire::set_vcd_value_direct(const struct value &v)
{
	vcd_locked = true;
	record_value_change(v);
	value = v;
}

std::string wire::get_label(uint64_t val) const
{
	if (!label_table)
		return "";
	return label_table->lookup(val, width);
}

void wire::apply_vcd_value(const struct value &v)
{
	vcd_locked = true;
	record_value_change(v);
	value = v;
	if (net) {
		net->apply_vcd_value(v);
	} else {
		struct value copy = v;
		for (auto *p : ports) {
			if (p)
				p->update_value_final(&copy, DEFAULT_TTL);
		}
	}
}

void wire::notify(int ttl)
{
}


static bool segment_intersects_unit_box(float x1, float y1, float x2, float y2,
                                        float bx, float by)
{
	/* Liang-Barsky test of segment P(t) = (x1, y1) + t*(dx, dy), t in [0, 1],
	 * against the half-open unit cell [bx, bx + 1) x [by, by + 1). */
	float tmin = 0.0f;
	float tmax = 1.0f;
	const float dx = x2 - x1;
	const float dy = y2 - y1;

	auto clip = [&](float p, float q) -> bool {
		if (std::fabs(p) < 1e-7f)
			return q >= 0.0f;
		const float r = q / p;
		if (p < 0.0f) {
			if (r > tmax) return false;
			if (r > tmin) tmin = r;
		} else {
			if (r < tmin) return false;
			if (r < tmax) tmax = r;
		}
		return true;
	};

	if (!clip(-dx, x1 - bx)) return false;
	if (!clip( dx, (bx + 1.0f) - x1)) return false;
	if (!clip(-dy, y1 - by)) return false;
	if (!clip( dy, (by + 1.0f) - y1)) return false;

	const float tx = x1 + tmin * dx;
	const float ty = y1 + tmin * dy;
	if (tx >= bx && tx < bx + 1.0f && ty >= by && ty < by + 1.0f)
		return true;
	if (tmax > tmin) {
		const float tm = 0.5f * (tmin + tmax);
		const float mx = x1 + tm * dx;
		const float my = y1 + tm * dy;
		return (mx >= bx && mx < bx + 1.0f && my >= by && my < by + 1.0f);
	}
	return false;
}

bool wire::intersect(float targetX, float targetY)
{
	if (!points || points->size() < 2)
		return false;

	constexpr float kHitRadiusSq = 0.49f * 0.49f;
	const auto &pts = *points;

	for (size_t i = 1; i < pts.size(); ++i) {
		const float x1 = pts[i - 1].X + 0.5f;
		const float y1 = pts[i - 1].Y + 0.5f;
		const float x2 = pts[i].X + 0.5f;
		const float y2 = pts[i].Y + 0.5f;

		/* Fast bounding-box reject covering both the 0.49 radius circle and [target, target+1) cell. */
		if (std::max(x1, x2) < targetX - 0.5f || std::min(x1, x2) > targetX + 1.0f ||
		    std::max(y1, y2) < targetY - 0.5f || std::min(y1, y2) > targetY + 1.0f)
			continue;

		const float dx = x2 - x1;
		const float dy = y2 - y1;
		const float len2 = dx * dx + dy * dy;
		float t = 0.0f;
		if (len2 > 1e-6f)
			t = std::clamp(((targetX - x1) * dx + (targetY - y1) * dy) / len2, 0.0f, 1.0f);
		const float cx = x1 + t * dx;
		const float cy = y1 + t * dy;
		const float ex = targetX - cx;
		const float ey = targetY - cy;
		if (ex * ex + ey * ey <= kHitRadiusSq)
			return true;

		if (segment_intersects_unit_box(x1, y1, x2, y2, targetX, targetY))
			return true;
	}

	return false;
}

float wire::distance_sq_to_point(float targetX, float targetY) const
{
	if (!points || points->size() < 2)
		return std::numeric_limits<float>::max();

	const auto &pts = *points;
	float best = std::numeric_limits<float>::max();

	for (size_t i = 1; i < pts.size(); ++i) {
		const float x1 = pts[i - 1].X + 0.5f;
		const float y1 = pts[i - 1].Y + 0.5f;
		const float x2 = pts[i].X + 0.5f;
		const float y2 = pts[i].Y + 0.5f;

		const float dx = x2 - x1;
		const float dy = y2 - y1;
		const float len2 = dx * dx + dy * dy;
		float t = 0.0f;
		if (len2 > 1e-6f)
			t = std::clamp(((targetX - x1) * dx + (targetY - y1) * dy) / len2, 0.0f, 1.0f);
		const float cx = x1 + t * dx;
		const float cy = y1 + t * dy;
		const float ex = targetX - cx;
		const float ey = targetY - cy;
		const float d2 = ex * ex + ey * ey;
		if (d2 < best)
			best = d2;
	}

	return best;
}

void wire::remove(void)
{
	/* Hold a strong reference while detaching from both endpoints so `this`
	 * cannot be destroyed before the loop finishes. */
	auto self = shared_from_this();
	class net *old_net = net;
	if (old_net) {
		old_net->remove_wire(this);
		net = nullptr;
	}
	auto ports_copy = ports;
	ports.clear();
	for (auto port : ports_copy) {
		if (!port)
			continue;
		port->remove_wire(this);
		if (old_net) {
			bool still_on_net = false;
			for (const auto &pw : port->get_wires()) {
				if (pw && pw->get_net() == old_net) {
					still_on_net = true;
					break;
				}
			}
			if (!still_on_net)
				old_net->remove_port(port);
		}
	}
	points.reset();
}


/* returns a new wire. After this call both
   the current and new wire have one loose side */
std::shared_ptr<class wire> wire::split(void)
{
	auto wr = std::make_shared<wire>(0, 0, 0, 0, color);

	logger::get().debug("Splitting wire");
	wr->update_value_final(&value, 100);    
	wr->set_width(get_width());
	if (!vcd_valid_name.empty())
		wr->set_vcd_valid_signal(vcd_valid_name, vcd_valid_active_low);
	if (label_table)
		wr->set_label_table(label_table);
	if (timing.critical_path)
		wr->set_timing_info(timing);
	/* upper bound the distance from outports */
	wr->set_distance_from_outport(distance_from_outport + 1);
	distance_from_outport++;
	if (ports.size() > 0) {
		ports[0]->replace_wire(this, wr);
		ports.erase(ports.begin() + 0);
	}
	wr->reseat();
	wr->push_wire_color(color);
	reseat();

	return wr;
}

void wire::to_json(json &j)
{
	j["wire_id"] = std::format("{:x}", reinterpret_cast<uintptr_t>(this));
	j["value"] = value;
	j["name"] = name;
	j["X1"] = X1;
	j["Y1"] = Y1;
	j["X2"] = X2;
	j["Y2"] = Y2;
	j["color"] = color;
	j["width"] = width;
	j["distance_from_outport"] = distance_from_outport;
	j["is_reversed"] = is_reversed;
	if (!vcd_name.empty())
		j["vcd_name"] = vcd_name;
	if (!vcd_valid_name.empty())
		j["vcd_valid_name"] = vcd_valid_name;
	if (vcd_valid_active_low)
		j["vcd_valid_active_low"] = true;
	if (vcd_locked)
		j["vcd_locked"] = true;
	if (!src_location.empty())
		j["src_location"] = src_location;
	if (!src_verilog.empty())
		j["src_verilog"] = src_verilog;
	if (timing.critical_path) {
		j["critical_path"] = true;
		if (timing.critical_rank > 0)
			j["critical_rank"] = timing.critical_rank;
		if (timing.has_delay)
			j["timing_delay_ns"] = timing.delay_ns;
		if (timing.has_slack)
			j["timing_slack_ns"] = timing.slack_ns;
		if (timing.logic_depth > 0)
			j["logic_depth"] = timing.logic_depth;
	}
	if (label_table && !label_table->empty()) {
		if (!label_table->get_name().empty() &&
		    label_table::find_registered(label_table->get_name()) != nullptr) {
			j["label_table_name"] = label_table->get_name();
		} else {
			j["label_table"] = label_table->to_json();
		}
	}
	if (points)
		j["points"] = *points;
}
void wire::from_json(json &j)
{
	value.is_clock = false;
	value = j["value"];
	name = j["name"];
	X1 = j["X1"];
	Y1 = j["Y1"];
	X2 = j["X2"];
	Y2 = j["Y2"];
	color = j["color"];
	width = j.value("width", 0);
	is_reversed = j.value("is_reversed", false);
	distance_from_outport = j.value("distance_from_outport", INT_MAX);
	vcd_name = j.value("vcd_name", "");
	vcd_valid_name = j.value("vcd_valid_name", "");
	vcd_valid_active_low = j.value("vcd_valid_active_low", false);
	vcd_locked = j.value("vcd_locked", false);
	src_location = j.value("src_location", "");
	src_verilog = j.value("src_verilog", "");
	timing = {};
	timing.critical_path = j.value("critical_path", false);
	timing.critical_rank = j.value("critical_rank", 0);
	if (j.contains("timing_delay_ns") && j["timing_delay_ns"].is_number()) {
		timing.delay_ns = j["timing_delay_ns"].get<float>();
		timing.has_delay = true;
	}
	if (j.contains("timing_slack_ns") && j["timing_slack_ns"].is_number()) {
		timing.slack_ns = j["timing_slack_ns"].get<float>();
		timing.has_slack = true;
	}
	timing.logic_depth = j.value("logic_depth", 0);
	label_table.reset();
	if (j.contains("label_table_name") && j["label_table_name"].is_string()) {
		label_table = label_table::find_registered(j["label_table_name"].get<std::string>());
	} else if (j.contains("label_table") && j["label_table"].is_object()) {
		auto tbl = label_table::from_json("", j["label_table"]);
		if (tbl && !tbl->empty())
			label_table = std::move(tbl);
	}
	if (j.contains("points")) {
		points = std::make_unique<std::vector<struct waypoint>>();
		*points = j["points"];
	}
}


static std::vector<std::map<std::string, std::shared_ptr<class wire>>> json_wire_scopes;

static std::map<std::string, std::shared_ptr<class wire>> &active_wire_scope(void)
{
	if (json_wire_scopes.empty())
		json_wire_scopes.emplace_back();
	return json_wire_scopes.back();
}

wire_factory_scope::wire_factory_scope(void)
{
	json_wire_scopes.emplace_back();
}

wire_factory_scope::~wire_factory_scope(void)
{
	if (!json_wire_scopes.empty())
		json_wire_scopes.pop_back();
}

void clear_wire_factory(void)
{
	if (!json_wire_scopes.empty())
		json_wire_scopes.back().clear();
}

bool wire_factory_force_new_name;

std::shared_ptr<class wire> json_wire_factory(json &jwire)
{
	auto &scope = active_wire_scope();
	/* Key by per-wire unique wire_id so multi-segment nets sharing a Verilog name don't collide. */
	std::string key = jwire.contains("wire_id") && jwire["wire_id"].is_string()
	                      ? jwire["wire_id"].get<std::string>()
	                      : jwire.value("name", "");

	auto it = scope.find(key);
	if (it != scope.end()) {
		auto _wire = it->second;
		scope.erase(it);
		return _wire;
	}
	auto new_wire = std::make_shared<wire>(jwire["X1"], jwire["Y1"], jwire["X2"], jwire["Y2"]);

	scope[key] = new_wire;

	new_wire->from_json(jwire);

	if (wire_factory_force_new_name)
		new_wire->set_new_name();

	return new_wire;
}



static void grid_line(class wiregrid *grid, float x1, float y1, float x2, float y2, bool is_clock)
{
	const float self_cost = (is_clock && grid->is_clock) ? -0.05f : 2.25f;
	const float d = dist(x1, y1, x2, y2);
	if (d < 0.01f)
		return;

	/* Step once per integer grid cell along the segment (avoids 12x oversampling + inner sqrt). */
	const int steps = std::max(1, static_cast<int>(std::ceil(d)));
	const float dx = (x2 - x1) / static_cast<float>(steps);
	const float dy = (y2 - y1) / static_cast<float>(steps);

	for (int s = 0; s <= steps; ++s) {
		const int gx = static_cast<int>(x1);
		const int gy = static_cast<int>(y1);
		if (!is_clock)
			grid->declare_part_of_wire(gx, gy);
		if (grid->get_soft_cost(gx, gy) < 0.001f)
			grid->add_soft_cost(gx, gy, self_cost);
		if (grid->get_soft_cost(gx - 1, gy) < 0.001f)
			grid->add_soft_cost(gx - 1, gy, -0.05);
		if (grid->get_soft_cost(gx + 1, gy) < 0.001f)
			grid->add_soft_cost(gx + 1, gy, -0.05);
		if (grid->get_soft_cost(gx, gy - 1) < 0.001f)
			grid->add_soft_cost(gx, gy - 1, -0.05);
		if (grid->get_soft_cost(gx, gy + 1) < 0.001f)
			grid->add_soft_cost(gx, gy + 1, -0.05);
		x1 += dx;
		y1 += dy;
	}
}

void wire::fill_grid(class wiregrid *grid)
{
	int prevX, prevY;
	bool first = true;

	if (!points)
		return;
	if (being_routed)
		return;

	first =true;
	for (auto point: *points) {
		if (first) {
			prevX = point.X;
			prevY = point.Y;
			first = false;
			continue;
		}
		grid_line(grid, prevX + 0.5, prevY + 0.5, point.X + 0.5, point.Y + 0.5, is_rail_wire(this));
		prevX = point.X;
		prevY = point.Y;
	}
}

void wire::remove_if_orphan(void)
{
	if (ports.size() < 2) {
		logger::get().debug("Removing orphan  {}", ports.size());
		remove();
	}
}

class port *wire::get_other_port(class port *port)
{
	for (auto p : ports) {
		if (p != port)
			return p;
	}
	return nullptr;
}

void wire::push_wire_color(int _color)
{
	if (color == _color)
		return;
	color = _color;
	return;
	for (auto port : ports)
		port->push_wire_color(color);
}

std::string wire::get_verilog_decl(void)
{
	std::string s;

	s = "wire ";
	if (width > 1)
		s = s + "[" + std::to_string(width -1) + ":0] ";

	s = s  + get_verilog_name();
	s = s + ";";

	return s;
}

std::string wire::get_verilog_name(void)
{
	std::string s = name;
	std::replace(s.begin(), s.end(), '-', '_');
	s = s + std::to_string(distance_from_outport);
	return s;

}

void wire::add_net(class net *newnet)
{
	if (net == newnet)
		return;

	net = newnet;

	for (auto port : ports)
		port->add_net(newnet);

	net->add_wire(this);
}

void wire::remove_net(void)
{
	class net *save;
	if (!net)
		return;
	save = net;
	net = nullptr;
	for (auto port : ports) {
		port->remove_net();	 
		save->remove_port(port);
	}

	save->remove_wire(this);
}

bool wire::update_distances(void)
{	
	unsigned int best = INT_MAX;

	for (auto port:ports)
		if (port->get_distance_from_outport() < best)
			best = port->get_distance_from_outport();

	if (distance_from_outport > best + 1) {
		distance_from_outport = best + 1;
		return true;
	}
	return false;
}

/* credit: https://easings.net/#easeInOutBack */
float ratio_effect(float ratio)
{
	float c1 = 1.70157;
	float c2 = c1 * 1.525;
	float r;
	if (ratio <= 0)
		return 0.0;
	if (ratio >= 1.0)
		return 1.0;
		
	if (ratio < 0.5)
		r = (pow(2 * ratio, 2) * ((c2 + 1) * 2 * ratio - c2))/2;
	else
		r = (pow(2 * ratio - 2, 2) * ((c2 + 1) * (ratio * 2 -2) + c2) + 2) /2;
		
	if (r < - 1)
		r = -1;
	if (r > 2)
		r = 2;
	return r;
}

void wire::add_midpoint(float x1, float y1, float x2, float y2, float reffect)
{
	struct waypoint wp = {};
	float dx, dy;
	dx = x2 - x1;
	dy = y2 - y1;
	wp.X = x1 + reffect * dx;
	wp.Y = y1 + reffect * dy;
	drawpoints->push_back(wp);
}

extern float animation_time;

void wire::calculate_drawpoints(void)
{
	uint64_t now = 0;
	float ratio = 1.0;
	float reffect = 1.0;
	struct waypoint wp = {};
	std::unique_ptr<std::vector<struct waypoint>> temppoints;

	if (drawpoints && !in_animation)
		return;
	drawpoints.reset();
	if (!points)
		return;
	if (points->size() < 2)
		return;
	
	
	now = SDL_GetTicks();
	
	ratio = (now - points_timestamp) / animation_time;
	if (ratio < 0.0)
		ratio = 0.0;
	if (ratio > 1.0)
		ratio = 1.0;
	
	/* if we end up with a too different wire, just zap the old points */
	
	if (oldpoints) {
		if ((*oldpoints)[0].X != (*points)[0].X || (*oldpoints)[0].Y != (*points)[0].Y) {
			unsigned int q = points->size() - 1;
			unsigned int p = oldpoints->size() - 1;
			if ((*oldpoints)[p].X == (*points)[q].X && (*oldpoints)[p].Y == (*points)[q].Y) {
					/* just animate the wire to the new spot */
			} else {
				oldpoints.reset();
			}
		}
	}
			
	if (oldpoints && oldpoints->size() < points->size() && oldpoints->size() > 2) {
		while (oldpoints->size() < points->size()) {
			unsigned int i = 1 + (rand() % (oldpoints->size()-1));
			wp.X = ((*oldpoints)[i].X + (*oldpoints)[i - 1].X)/2;
			wp.Y = ((*oldpoints)[i].Y + (*oldpoints)[i - 1].Y)/2;
			oldpoints->insert(oldpoints->begin() + i, wp);
		}
	}

	if (oldpoints && oldpoints->size() > points->size() && points->size() >= 2) {
	
		if (points->size() == 2) {
			temppoints = std::move(points);
			points = std::make_unique<std::vector<struct waypoint>>();
			float x1, y1, dx, dy, totaldist = 0, runningdist = 0;
		
			for (unsigned int i = 1; i < oldpoints->size(); i++)
			{
				totaldist += dist((*oldpoints)[i-1].X, (*oldpoints)[i-1].Y, (*oldpoints)[i].X, (*oldpoints)[i].Y);
			}
		
			if (totaldist == 0)
				totaldist = 1;
		
		
			x1 = (*temppoints)[0].X;
			y1 = (*temppoints)[0].Y;
			dx = (*temppoints)[1].X - x1;
			dy = (*temppoints)[1].Y - y1;
			for (unsigned int i = 0; i < oldpoints->size(); i++) {
				if (i == 0) {
					wp = (*temppoints)[i];
					points->push_back(wp);
					continue;
				}
				if (i == oldpoints->size() - 1) {
					wp = (*temppoints)[1];
					points->push_back(wp);
					continue;
				}
				runningdist += dist((*oldpoints)[i-1].X, (*oldpoints)[i-1].Y, (*oldpoints)[i].X, (*oldpoints)[i].Y);
				wp.X = x1 + dx * (runningdist/totaldist);
				wp.Y = y1 + dy * (runningdist/totaldist);
				points->push_back(wp);
			}
		} else {

			temppoints = std::move(points);
			points = std::make_unique<std::vector<struct waypoint>>();
			for (auto i : *temppoints)
				points->push_back(i);
			
			while (oldpoints->size() > points->size()) {
				unsigned int i = 1 + (rand() % (points->size()-1));
				wp.X = ((*points)[i].X + (*points)[i - 1].X)/2;
				wp.Y = ((*points)[i].Y + (*points)[i - 1].Y)/2;
				points->insert(points->begin() + i, wp);
			}
		}
	}
	if (oldpoints && oldpoints->size() != points->size()) {
		oldpoints.reset();
	}
	
	if (!oldpoints) {
		oldpoints = std::make_unique<std::vector<struct waypoint>>();
		float x1, y1, dx, dy, totaldist = 0, runningdist = 0;
		
		for (unsigned int i = 1; i < points->size(); i++)
		{
			totaldist += dist((*points)[i-1].X, (*points)[i-1].Y, (*points)[i].X, (*points)[i].Y);
		}
		
		if (totaldist == 0)
			totaldist = 1;
		
		
		x1 = (*points)[0].X;
		y1 = (*points)[0].Y;
		dx = (*points)[points->size()-1].X - x1;
		dy = (*points)[points->size()-1].Y - y1;
		for (unsigned int i = 0; i < points->size(); i++) {
			if (i == 0 || i == points->size() - 1) {
				wp = (*points)[i];
				oldpoints->push_back(wp);
				continue;
			}
			runningdist += dist((*points)[i-1].X, (*points)[i-1].Y, (*points)[i].X, (*points)[i].Y);
			wp.X = x1 + dx * (runningdist/totaldist);
			wp.Y = y1 + dy * (runningdist/totaldist);
			oldpoints->push_back(wp);
		}
	}
	

	drawpoints = std::make_unique<std::vector<struct waypoint>>();
	
	reffect = ratio_effect(ratio);

	for (unsigned int i = 0; i < points->size(); i++)
	{		
		if (i == 0) {
			wp = (*points)[i];
			drawpoints->push_back(wp);
			continue;
		}

		if (i == points->size() - 1) {
			wp = (*points)[i];
			drawpoints->push_back(wp);
			continue;
		}
		
		auto p = (*points)[i];
		auto o = (*oldpoints)[i];
		add_midpoint(o.X, o.Y, p.X, p.Y, reffect);
	}
	
	if (ratio >= 1.0)
		in_animation = false;
		
	if (temppoints) {
		points = std::move(temppoints);
	}
}


void wire::redo_wires(void)
{
	if (!points)
		return;
	oldpoints = std::move(points);
}
