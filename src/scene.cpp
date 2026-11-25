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

#include <stdio.h>

#include "gridcad.h"
#include "scene.h"
#include "placer.h"
#include "layout_overlay.h"
#include "element.h"
#include "connector.h"
#include "port.h"
#include "wire.h"
#include "wirepath.h"
#include "net.h"
#include "model_nest.h"
#include "contextmenu.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <queue>
#include <unordered_set>

void callback_select_all(class scene *scene)
{
	for (auto &elem : scene->elements) {
		elem->select();
	}
}

void callback_fit_to_screen(class scene *scene)
{
	SDL_Event ev = {};


	ev.type = EVENT_ZOOM_TO_FIT;
	ev.user.code = 0;
	ev.user.data1 = scene;

	SDL_PushEvent(&ev);
}

void callback_fit_to_screen2(class scene *scene)
{
	SDL_Event ev = {};


	ev.type = EVENT_ZOOM_TO_FIT;
	ev.user.code = 1;
	ev.user.data1 = scene;

	SDL_PushEvent(&ev);
}

void callback_sequencer_reset(class scene *scene)
{
	SDL_Event ev = {};


	ev.type = EVENT_SEQUENCER_RESET;
	ev.user.code = 0;
	ev.user.data1 = scene;

	SDL_PushEvent(&ev);
}

void callback_autoclock(class scene *scene)
{
	class port *clk = nullptr;
	for (auto &elem : scene->elements) {
		clk = elem->get_clk_port();
		if (clk) 
			break;
	}
	if (!clk)
		return;
	for (auto &elem : scene->elements) {
		elem->connect_clk(clk);
	}
}

void callback_reroute(class scene *scene)
{
	scene->reroute_all_wires();
}

void callback_redo_connectors(class scene *scene)
{
	topological_placer::place_connectors(scene);
	scene->reroute_all_wires();
}

void callback_reset_layout(class scene *scene)
{
	layout_overlay::reset_scene_layout(scene);
}

void callback_validate(class scene *scene)
{
	scene->validate_ports();
}

scene::scene(std::string _name, std::string _parent)
{
	sizeX = 200;
	sizeY = 200;
	name = _name;
	parental_name = _parent;
	menu = std::make_unique<class contextmenu>(this);
	menu->add_item("Fit to Screen", callback_fit_to_screen);
	menu->add_item("Select All", callback_select_all);
	menu->add_item("Connect clocks", callback_autoclock);
	menu->add_item("Reroute wires", callback_reroute);
	menu->add_item("Redo connector placement", callback_redo_connectors);
	menu->add_item("Reset saved layout", callback_reset_layout);
	menu->add_item("Validate routing", callback_validate);
	menu->add_item("Reset all sequencers", callback_sequencer_reset);
}

scene::~scene(void)
{
	remove_nets();
	for (auto &elem : elements) {
		remove_from_calculate_queue(elem.get());
	}
}



void scene::add_element(std::unique_ptr<class element> element)
{
	class element *raw = element.get();
	elements.push_back(std::move(element));
	rewire_section(raw->get_X(), raw->get_Y(), raw->get_width(), raw->get_height());
	generation_count++;
	raw->update_parental_name(get_full_name());
	logger::get().debug("Element parental name is {}   name is {} parent is {}", raw->get_parental_name(), name, parental_name);
	
	has_sequencer = false;
	for (auto &elem : elements)
		if (elem->class_id() == "sequencer:")
			has_sequencer = true;
			
	if (has_sequencer)
		menu->set_active("Reset all sequencers");
	else
		menu->set_inactive("Reset all sequencers");
}

bool scene::can_place_element(float x, float y, int w, int h,
		class element * myself)
{
	for (auto const &elem : elements) {
		int _x, _y;
		if (elem.get() == myself)
			continue;
		if (elem->is_background()) /* labels are special and can be placed upon */
			continue;
		for (_y = -2; _y < h + 2; _y++)
			for (_x = -2; _x < w + 2 ; _x++)
				if (myself->intersect_float(x + _x, y + _y) && elem->intersect_full(x + _x, y + _y)) {
					return false;
				}
	}
	return true;
}

class port *scene::is_port(float X, float Y)
{
	for (auto const &elem : elements) {
		class port *p = elem->is_port(X, Y);
		if (p)
			return p;
	}
	return nullptr;

}

std::shared_ptr<class wire> scene::is_wire(float X, float Y)
{
	/* Pick the closest wire centerline within a symmetric hit radius so parallel 45-deg
	 * wires never steal hits from each other due to iteration order or asymmetric cell boxes. */
	constexpr float kHitRadiusSq = 0.45f * 0.45f;
	std::shared_ptr<class wire> best_wire;
	float best_d2 = kHitRadiusSq;

	for (auto const &elem : elements) {
		if (!elem)
			continue;
		for (unsigned int pi = 0;; ++pi) {
			port *p = elem->port_at(pi);
			if (!p)
				break;
			for (const auto &w : p->get_wires()) {
				if (!w)
					continue;
				const float d2 = w->distance_sq_to_point(X, Y);
				if (d2 < best_d2) {
					best_d2 = d2;
					best_wire = w;
				}
			}
		}
	}
	return best_wire;
}

void scene::fill_grid(class wiregrid * grid)
{
	for (auto const &elem : elements)
		elem->fill_grid(grid);
}

void scene::deselect_all(void)
{
	for (auto &element : elements)
		element->deselect();
}
class element * scene::selected_element(void)
{
	for (auto &element : elements)
		if (element->is_selected())
			return element.get();
	return nullptr;
}

void scene::remove_element(class element *element)
{
	unsigned int i;
	for (i = 0; i < elements.size(); i++) {
		if (elements[i].get() == element) {
			remove_from_calculate_queue(element);
			elements.erase(elements.begin() + i); /* must exit the for loop now */
			return;
		}
	}
	generation_count++;
	has_sequencer = false;
	for (auto &elem : elements)
		if (elem->class_id() == "sequencer:")
			has_sequencer = true;
	if (has_sequencer)
		menu->set_active("Reset all sequencers");
	else
		menu->set_inactive("Reset all sequencers");
}

void scene::to_json(json &j)
{
	unsigned int i;
	j["sizeX"] = sizeX;
	j["sizeY"] = sizeY;
	j["elements"] = json::array();
	j["name"] = name;
	j["parental_name"] = parental_name;
	j["verilog_name"] = verilog_name;
	j["has_sequencer"] = has_sequencer;
	if (!layout_project_key.empty())
		j["layout_project_key"] = layout_project_key;
	if (!layout_module_name.empty())
		j["layout_module_name"] = layout_module_name;
	if (layout_dirty)
		j["layout_dirty"] = true;
	if (needs_steiner_routing)
		j["needs_steiner_routing"] = true;

	for (i = 0; i < elements.size(); i++) {
		json p;
		elements[i]->to_json(p);
		j["elements"][i] = p;
	}
}

void scene::selection_to_json(json &j)
{
	unsigned int i;
	unsigned int q = 0;
	j["elements"] = json::array();

	name = j.value("name", "");
	parental_name = j.value("parental_name", "");
	verilog_name = j.value("verilog_name", "");

	for (i = 0; i < elements.size(); i++) {
		if (elements[i]->is_selected()) {
			json p;
			elements[i]->to_json(p);
			j["elements"][q++] = p;
		}
	}
}

void scene::from_json(json &j)
{
	/* Isolate wire-ID pairing per scene so recursive model_nest loads never collide. */
	wire_factory_scope wire_scope;
	unsigned int i;

	sizeX = j["sizeX"];
	sizeY = j["sizeY"];
	if (j.contains("name") && j["name"].is_string() && !j["name"].get<std::string>().empty())
		name = j["name"].get<std::string>();
	parental_name = j.value("parental_name", parental_name);
	verilog_name = j.value("verilog_name", verilog_name);
	layout_project_key = j.value("layout_project_key", "");
	layout_module_name = j.value("layout_module_name", layout_module_name);
	layout_dirty = j.value("layout_dirty", false);
	needs_steiner_routing = j.value("needs_steiner_routing", false);

	for (i = 0; i <j["elements"].size(); i++) {
		json p = j["elements"][i];
		std::string cid = p["class_id"];
		auto element = element_from_class_id(cid);
		element->from_json(p);
		element->update_parental_name(get_full_name());
		elements.push_back(std::move(element));
	}
	for (auto &element : elements)
		element->calculate(100);

	if (!is_import_view_mode() && layout_project_key.empty())
		cycle_color();
	redo_nets();
	has_sequencer = j.value("has_sequencer", false);
	if (has_sequencer)
		menu->set_active("Reset all sequencers");
	else
		menu->set_inactive("Reset all sequencers");
}

void scene::process_delete_requests(void)
{
	std::vector<class element *> todo;

	for (auto &elem : elements) {
		if (elem->want_deleted())
			todo.push_back(elem.get());
	}

	for (auto elem : todo) {
		remove_element(elem);
		generation_count++;
	}
}

void scene::delete_selection(void)
{
	for (auto &elem : elements)
		if (!elem->in_edit_mode())
			elem->delete_if_selected();
}


void scene::rewire_section(int x1, int y1, int w, int h)
{
	int x, y;

	for (y = y1 - 1 ; y <= y1 + h + 1; y++)
		for (x = x1 -1 ; x <= x1 + w + 1; x++) {
			auto wire = is_wire(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f);
			if (wire) {
				wire->reseat();
				wire->redo_wires();
			}
		}
}

void scene::rewire_section(class element *element)
{
	rewire_section(element->get_X(), element->get_Y(), element->get_width(), element->get_height());
}

void scene::remove_orphans(void)
{
	for (auto &elem : elements)
		elem->remove_orphans();
}

unsigned int scene::selected_count(void)
{
	unsigned int count = 0;
	for (auto &elem : elements)
		if (elem->is_selected()) 
			count++;

	return count;
}


void scene::cycle_color(void)
{
	for (auto &elem : elements)
		elem->cycle_color();
	redo_nets();
}

void scene::reroute_all_wires(void)
{
	for (auto &elem : elements)
		elem->reroute_all_wires();
}

void scene::create_verilog_names(void)
{
	if (verilog_name == "") {
		verilog_name = parental_name + "_" + name;
		if (vname != "")
			verilog_name = parental_name + "_" + vname;
		verilog_name = append_random_bits(verilog_name + "_");

		if (parental_name == "" && name == "main")
			verilog_name = "main";
	}
	std::replace(verilog_name.begin(), verilog_name.end(), '-', '_');
	std::replace(verilog_name.begin(), verilog_name.end(), '+', '_');
	std::replace(verilog_name.begin(), verilog_name.end(), '/', '_');

	std::vector<std::string> existing;
	for (unsigned int i = 0; i < elements.size(); i++) {
		auto &elem = elements[i];
		elem->create_verilog_name(i, &existing);
	}
}

std::string scene::get_verilog_main(void)
{
	std::string s = "";

	redo_nets();

	if (name == "")
		name = "main_scene";

	create_verilog_names();

	s = "module " + verilog_name + "\n";

	s += "(";
	/* in */
	bool first = true;
	for (auto &elem : elements) {
		if (elem->class_id() == "model_toggle:") {
			if (!first)
				s += ", ";
			first = false;
			s += "input " + elem->get_verilog_width() + " " +elem->get_verilog_name();
		}
		if (elem->class_id() == "model_clock:") {
			if (!first)
				s += ", ";
			first = false;
			s += "input " + elem->get_verilog_width() + " " +elem->get_verilog_name();
		}
		if (elem->class_id() == "model_output:") {
			if (!first)
				s += ", ";
			first = false;
			s += "output " + elem->get_verilog_width() + " " +elem->get_verilog_name();
		}
	}	
	s += ");\n\n";


	std::vector<std::string> netmap;

	for (auto &elem : elements) {
		elem->collect_nets(&netmap);
	}

	for (const auto &netstr : netmap)
		s += std::format("{}\n", netstr);

	s += "\n";

	for (auto &elem : elements) {
		s += elem->get_verilog_main();
	}



	s += "endmodule\n\n";



	return s;
}


std::string scene::get_verilog_modules(std::string verilog_path)
{
	std::string s = "";

	for (auto &elem : elements) {
		s += elem->get_verilog_modules(verilog_path);
	}

	return s;
}
std::string scene::get_full_name(void) 
{ 
	if (vname != "")
		return parental_name + "/" + vname;
	return parental_name + "/" + name;
};


void scene::remove_nets(void)
{
	for (auto &elem : elements)
		elem->remove_nets();
	for (auto &elem : elements)
		elem->free_nets_memory();
}

void scene::add_nets(void)
{
	for (auto &elem : elements)
		elem->add_nets();
}

void scene::sync_nets(void)
{
	for (auto &elem : elements)
		elem->sync_nets();
}

void scene::redo_nets(void)
{
	remove_nets();
	add_nets();
	sync_nets();
}

void scene::redo_net(class element *seed_elem)
{
	if (!seed_elem)
		return;
	port *seed_port = seed_elem->port_at(0);
	if (!seed_port)
		return;

	/* 1. BFS across wires & connectors to collect every wire and real endpoint on this net. */
	std::unordered_set<const wire *> seen_wires;
	std::unordered_set<const port *> seen_ports;
	std::vector<std::shared_ptr<wire>> old_wires;
	std::vector<port *> drv_ports;
	std::vector<port *> sink_ports;
	std::queue<port *> pq;

	pq.push(seed_port);
	seen_ports.insert(seed_port);

	while (!pq.empty()) {
		port *cur = pq.front();
		pq.pop();
		const bool is_conn_port = cur->parent && (cur->parent->class_id() == "connector:");
		if (!is_conn_port) {
			if (cur->direction == PORT_OUT)
				drv_ports.push_back(cur);
			else
				sink_ports.push_back(cur);
		}
		for (const auto &w : cur->get_wires()) {
			if (!w || !seen_wires.insert(w.get()).second)
				continue;
			old_wires.push_back(w);
			for (port *ep : w->get_ports()) {
				if (ep && seen_ports.insert(ep).second) {
					const bool ep_conn = ep->parent && (ep->parent->class_id() == "connector:");
					if (ep_conn) {
						pq.push(ep);
					} else {
						if (ep->direction == PORT_OUT)
							drv_ports.push_back(ep);
						else
							sink_ports.push_back(ep);
					}
				}
			}
		}
	}

	if (old_wires.empty() || (drv_ports.size() + sink_ports.size()) < 2)
		return;

	/* Snapshot net metadata from the richest wire before tearing down old wires/connectors. */
	std::shared_ptr<wire> tmpl = old_wires[0];
	for (const auto &w : old_wires) {
		if (!w->get_vcd_name().empty() || !w->get_src_location().empty()) {
			tmpl = w;
			break;
		}
	}
	const std::string meta_name = tmpl->name;
	const int meta_color = tmpl->get_color();
	const int meta_width = std::max(1, tmpl->get_width());
	const std::string meta_vcd = tmpl->get_vcd_name();
	const std::string meta_vcd_valid = tmpl->get_vcd_valid_name();
	const bool meta_vcd_valid_low = tmpl->is_vcd_valid_active_low();
	const bool meta_vcd_locked = tmpl->is_vcd_locked();
	const struct value meta_value = tmpl->value;
	const auto meta_lbl = tmpl->get_label_table();
	const std::string meta_src_loc = tmpl->get_src_location();
	const std::string meta_src_v = tmpl->get_src_verilog();
	const struct timing_info meta_timing = tmpl->get_timing_info();
	const bool is_rail = meta_value.is_clock || meta_name == "clk" ||
	                     meta_name == "rst" || meta_name == "rst_n";

	/* 2. Choose root (driver) and targets (all other real endpoints). */
	port *root = nullptr;
	std::vector<port *> targets;
	if (!drv_ports.empty()) {
		root = drv_ports[0];
		for (size_t i = 1; i < drv_ports.size(); ++i)
			targets.push_back(drv_ports[i]);
		for (port *sp : sink_ports)
			targets.push_back(sp);
	} else {
		root = sink_ports[0];
		for (size_t i = 1; i < sink_ports.size(); ++i)
			targets.push_back(sink_ports[i]);
	}

	/* 3. Remove all old wires and orphaned connectors on this net. */
	for (auto &w : old_wires)
		w->remove();
	old_wires.clear();
	process_delete_requests();

	/* Build forbidden bounding boxes for non-connector elements so connectors stay clear. */
	struct bbox {
		int x1, y1, x2, y2;
	};
	std::vector<bbox> blocked_boxes;
	std::unordered_set<int64_t> pin_cells;
	auto cell_key = [](int x, int y) -> int64_t {
		return (static_cast<int64_t>(x) << 32) | static_cast<uint32_t>(y);
	};
	for (const auto &elem : elements) {
		if (!elem || elem->is_background() || elem->class_id() == "connector:")
			continue;
		const int ox = static_cast<int>(std::floor(elem->get_X()));
		const int oy = static_cast<int>(std::floor(elem->get_Y()));
		const int ow = std::max(1, static_cast<int>(std::ceil(elem->get_width())));
		const int oh = std::max(1, static_cast<int>(std::ceil(elem->get_height())));
		blocked_boxes.push_back({ox - 1, oy - 1, ox + ow, oy + oh});
		for (unsigned int pi = 0;; ++pi) {
			port *p = elem->port_at(pi);
			if (!p)
				break;
			pin_cells.insert(cell_key(static_cast<int>(std::round(p->screenX)),
			                          static_cast<int>(std::round(p->screenY))));
		}
	}
	auto is_valid_tap = [&](int x, int y) -> bool {
		if (x < 1 || y < 1 || x >= static_cast<int>(sizeX) - 1 || y >= static_cast<int>(sizeY) - 1)
			return false;
		if (pin_cells.count(cell_key(x, y)))
			return false;
		for (const auto &b : blocked_boxes) {
			if (x >= b.x1 && x <= b.x2 && y >= b.y1 && y <= b.y2)
				return false;
		}
		return true;
	};

	auto make_net_wire = [&](port *a, port *b,
	                         std::unique_ptr<std::vector<struct waypoint>> routed_pts) {
		auto w = std::make_shared<wire>(
		    static_cast<int>(std::round(a->screenX)), static_cast<int>(std::round(a->screenY)),
		    static_cast<int>(std::round(b->screenX)), static_cast<int>(std::round(b->screenY)),
		    meta_color);
		w->name = meta_name;
		if (!meta_vcd.empty())
			w->set_vcd_name(meta_vcd);
		if (!meta_vcd_valid.empty())
			w->set_vcd_valid_signal(meta_vcd_valid, meta_vcd_valid_low);
		if (meta_lbl)
			w->set_label_table(meta_lbl);
		if (!meta_src_loc.empty() || !meta_src_v.empty())
			w->set_src_info(meta_src_loc, meta_src_v);
		if (meta_timing.critical_path)
			w->set_timing_info(meta_timing);
		w->value = meta_value;
		w->set_width(meta_width);
		a->add_wire(w);
		b->add_wire(w);
		w->push_wire_color(meta_color);
		if (routed_pts && !routed_pts->empty())
			w->set_routed_points(std::move(routed_pts));
		else
			w->route(this);
		return w;
	};

	/* 4. Score targets: farthest first + large bonus for pure horizontal / vertical alignment. */
	auto target_score = [&](port *t) -> double {
		const double dx = std::abs(t->screenX - root->screenX);
		const double dy = std::abs(t->screenY - root->screenY);
		double score = dx + dy;
		if (dy < 0.5)
			score += 500.0;
		if (dx < 0.5)
			score += 500.0;
		return score;
	};
	std::stable_sort(targets.begin(), targets.end(),
	                 [&](port *a, port *b) { return target_score(a) > target_score(b); });

	/* 5. Route primary backbone wire: root -> top-scored target. */
	std::vector<std::shared_ptr<wire>> net_wires;
	std::vector<connector *> net_connectors;
	net_wires.push_back(make_net_wire(root, targets[0], nullptr));

	/* 6. For each remaining target, run one multi-source A* from all valid cells on the net. */
	struct tap_info {
		wire *owner_wire = nullptr;
		size_t seg_idx = 0;
		connector *existing_conn = nullptr;
	};

	for (size_t ti = 1; ti < targets.size(); ++ti) {
		port *tgt = targets[ti];
		const int tx = static_cast<int>(std::round(tgt->screenX));
		const int ty = static_cast<int>(std::round(tgt->screenY));

		std::map<std::pair<int, int>, tap_info> tap_map;
		std::vector<wiregrid::seed_point> seeds;

		/* Seed existing connectors on this net with cost 0.0 (prefer reusing nearby junctions). */
		for (connector *ec : net_connectors) {
			const int cx = static_cast<int>(std::round(ec->get_X()));
			const int cy = static_cast<int>(std::round(ec->get_Y()));
			tap_map[{cx, cy}] = {nullptr, 0, ec};
			seeds.push_back({cx, cy, 0.0});
		}

		/* Seed every clear integer cell along already-routed wires of this net with cost 0.25. */
		for (const auto &nw : net_wires) {
			const auto *pts = nw->get_points();
			if (!pts || pts->size() < 2)
				continue;
			for (size_t s = 0; s + 1 < pts->size(); ++s) {
				const float x0 = (*pts)[s].X;
				const float y0 = (*pts)[s].Y;
				const float x1 = (*pts)[s + 1].X;
				const float y1 = (*pts)[s + 1].Y;
				const int steps = std::max(1, static_cast<int>(std::ceil(
				                                  std::max(std::abs(x1 - x0), std::abs(y1 - y0)))));
				for (int k = 0; k <= steps; ++k) {
					const float t = static_cast<float>(k) / static_cast<float>(steps);
					const int gx = static_cast<int>(std::round(x0 + (x1 - x0) * t));
					const int gy = static_cast<int>(std::round(y0 + (y1 - y0) * t));
					if (!is_valid_tap(gx, gy))
						continue;
					if (tap_map.find({gx, gy}) == tap_map.end()) {
						tap_map[{gx, gy}] = {nw.get(), s, nullptr};
						seeds.push_back({gx, gy, 0.25});
					}
				}
			}
		}

		/* Fallback if all trunk cells were adjacent to components: seed root or midpoint. */
		if (seeds.empty()) {
			const int rx = static_cast<int>(std::round(root->screenX));
			const int ry = static_cast<int>(std::round(root->screenY));
			seeds.push_back({rx, ry, 0.25});
		}

		wiregrid grid(static_cast<int>(sizeX), static_cast<int>(sizeY));
		grid.is_clock = is_rail;
		fill_grid(&grid);
		auto branch_pts = grid.path_walk_multi(seeds, tx, ty, is_rail);
		if (!branch_pts || branch_pts->empty()) {
			/* Safety fallback: direct route from root if multi-source failed. */
			net_wires.push_back(make_net_wire(root, tgt, nullptr));
			continue;
		}

		const int tap_x = static_cast<int>(std::round(branch_pts->front().X));
		const int tap_y = static_cast<int>(std::round(branch_pts->front().Y));
		auto it = tap_map.find({tap_x, tap_y});

		if (it != tap_map.end() && it->second.existing_conn) {
			/* Reuse existing connector at (tap_x, tap_y). */
			connector *ec = it->second.existing_conn;
			net_wires.push_back(make_net_wire(ec->port_at(0), tgt, std::move(branch_pts)));
		} else if (it != tap_map.end() && it->second.owner_wire) {
			/* Split owner_wire at (tap_x, tap_y) and insert a new connector. */
			wire *w_hit = it->second.owner_wire;
			const size_t seg_idx = it->second.seg_idx;

			const auto &eps = w_hit->get_ports();
			port *p_a = (eps.size() >= 1) ? eps[0] : nullptr;
			port *p_b = (eps.size() >= 2) ? eps[1] : nullptr;

			std::unique_ptr<std::vector<struct waypoint>> pts_a;
			std::unique_ptr<std::vector<struct waypoint>> pts_b;
			if (const auto *orig_pts = w_hit->get_points(); orig_pts && orig_pts->size() >= 2 && p_a && p_b) {
				/* Ensure p_a corresponds to orig_pts->front() and p_b to orig_pts->back(). */
				const float da = std::abs(p_a->screenX - orig_pts->front().X) +
				                 std::abs(p_a->screenY - orig_pts->front().Y);
				const float db = std::abs(p_b->screenX - orig_pts->front().X) +
				                 std::abs(p_b->screenY - orig_pts->front().Y);
				if (db < da)
					std::swap(p_a, p_b);

				pts_a = std::make_unique<std::vector<struct waypoint>>();
				for (size_t k = 0; k <= seg_idx && k < orig_pts->size(); ++k)
					pts_a->push_back((*orig_pts)[k]);
				if (pts_a->empty() ||
				    std::abs(pts_a->back().X - tap_x) > 0.1f ||
				    std::abs(pts_a->back().Y - tap_y) > 0.1f) {
					pts_a->push_back({static_cast<float>(tap_x), static_cast<float>(tap_y)});
				}

				pts_b = std::make_unique<std::vector<struct waypoint>>();
				pts_b->push_back({static_cast<float>(tap_x), static_cast<float>(tap_y)});
				for (size_t k = seg_idx + 1; k < orig_pts->size(); ++k) {
					if (k == seg_idx + 1 &&
					    std::abs((*orig_pts)[k].X - tap_x) < 0.1f &&
					    std::abs((*orig_pts)[k].Y - tap_y) < 0.1f) {
						continue;
					}
					pts_b->push_back((*orig_pts)[k]);
				}
			}

			/* Detach and remove w_hit from net_wires. */
			std::shared_ptr<wire> keep_hit;
			for (size_t wi = 0; wi < net_wires.size(); ++wi) {
				if (net_wires[wi].get() == w_hit) {
					keep_hit = net_wires[wi];
					net_wires.erase(net_wires.begin() + static_cast<ptrdiff_t>(wi));
					break;
				}
			}
			w_hit->remove();

			auto conn = std::make_unique<connector>(static_cast<float>(tap_x), static_cast<float>(tap_y));
			connector *conn_raw = conn.get();
			conn_raw->port_at(0)->set_width(meta_width);
			add_element(std::move(conn));
			net_connectors.push_back(conn_raw);

			if (p_a)
				net_wires.push_back(make_net_wire(p_a, conn_raw->port_at(0), std::move(pts_a)));
			if (p_b)
				net_wires.push_back(make_net_wire(conn_raw->port_at(0), p_b, std::move(pts_b)));
			net_wires.push_back(make_net_wire(conn_raw->port_at(0), tgt, std::move(branch_pts)));
		} else {
			/* Fallback: tap at root if no wire cell matched. */
			net_wires.push_back(make_net_wire(root, tgt, nullptr));
		}
	}

	/* 7. Rebuild nets and restore VCD lock if active. */
	redo_nets();
	if (meta_vcd_locked && !net_wires.empty() && net_wires[0]->get_net())
		net_wires[0]->get_net()->apply_vcd_value(meta_value);
}

void scene::redo_all_multi_sink_nets(void)
{
	/*
	 * Collect one representative connector per distinct multi-sink net (via BFS
	 * so chained trunk connectors belong to the same net), ordering:
	 *   Tier 0: clk/rst rails (square rails along the bottom)
	 *   Tier 1: forward (left-to-right) nets (avg sink X >= driver X), shorter spans first
	 *   Tier 2: backward (right-to-left feedback) nets (avg sink X < driver X) last so
	 *           feedback wraps around the already-laid forward datapath.
	 */
	struct net_seed {
		int tier = 1; /* 0 = rail, 1 = forward, 2 = backward */
		double span = 0.0;
		element *conn = nullptr;
	};
	std::unordered_set<const element *> visited_conns;
	std::vector<net_seed> net_seeds;

	for (const auto &elem : elements) {
		if (!elem || elem->class_id() != "connector:" || visited_conns.count(elem.get()))
			continue;

		bool is_rail = false;
		float drv_x = 0.0f;
		bool has_drv = false;
		float sink_x_sum = 0.0f;
		int sink_cnt = 0;

		std::queue<element *> q;
		q.push(elem.get());
		visited_conns.insert(elem.get());

		while (!q.empty()) {
			element *cur = q.front();
			q.pop();
			port *cp = cur->port_at(0);
			if (!cp)
				continue;
			for (const auto &w : cp->get_wires()) {
				if (!w)
					continue;
				if (w->value.is_clock || w->name == "clk" ||
				    w->name == "rst" || w->name == "rst_n") {
					is_rail = true;
				}
				port *other = w->get_other_port(cp);
				if (!other)
					continue;
				if (other->parent && other->parent->class_id() == "connector:") {
					if (visited_conns.insert(other->parent).second)
						q.push(other->parent);
				} else if (other->direction == PORT_OUT && !has_drv) {
					drv_x = other->screenX;
					has_drv = true;
				} else {
					sink_x_sum += other->screenX;
					sink_cnt++;
				}
			}
		}

		net_seed ns;
		ns.conn = elem.get();
		if (is_rail) {
			ns.tier = 0;
			ns.span = 0.0;
		} else if (has_drv && sink_cnt > 0) {
			const float avg_sink_x = sink_x_sum / static_cast<float>(sink_cnt);
			ns.tier = (avg_sink_x >= drv_x) ? 1 : 2;
			ns.span = std::abs(avg_sink_x - drv_x);
		} else {
			ns.tier = 1;
			ns.span = 0.0;
		}
		net_seeds.push_back(ns);
	}

	std::stable_sort(net_seeds.begin(), net_seeds.end(),
	                 [](const net_seed &a, const net_seed &b) {
		                 if (a.tier != b.tier)
			                 return a.tier < b.tier;
		                 return a.span < b.span;
	                 });

	for (const auto &ns : net_seeds) {
		/* Verify seed_conn is still alive in elements (not already deleted). */
		bool alive = false;
		for (const auto &e : elements) {
			if (e.get() == ns.conn) {
				alive = true;
				break;
			}
		}
		if (alive)
			redo_net(ns.conn);
	}
}

void scene::validate_ports(void)
{
	for (auto &elem : elements)
		elem->validate_ports();
}

size_t scene::broadcast_vcd_values(const std::unordered_map<std::string, struct value> &values)
{
	if (values.empty())
		return 0;

	size_t updated = 0;
	std::unordered_set<const net *> visited_nets;
	std::unordered_set<const wire *> visited_orphan_wires;

	for (auto &elem : elements) {
		if (elem->class_id() == "model_nest:") {
			auto *nest = static_cast<model_nest *>(elem.get());
			if (scene *sub = nest->get_scene())
				updated += sub->broadcast_vcd_values(values);
		}
		for (unsigned int pi = 0;; ++pi) {
			port *p = elem->port_at(pi);
			if (!p)
				break;
			for (const auto &w : p->get_wires()) {
				if (!w)
					continue;
				const std::string &vname = w->get_vcd_name();
				if (vname.empty())
					continue;
				if (net *n = w->get_net()) {
					if (!visited_nets.insert(n).second)
						continue;
				} else if (!visited_orphan_wires.insert(w.get()).second) {
					continue;
				}
				auto it = values.find(vname);
				if (it == values.end())
					continue;
				w->apply_vcd_value(it->second);
				updated++;
			}
		}
	}
	return updated;
}

size_t scene::broadcast_vcd_value(std::string_view vcd_name, const struct value &val)
{
	std::unordered_map<std::string, struct value> single;
	single.emplace(std::string(vcd_name), val);
	return broadcast_vcd_values(single);
}

void scene::unlock_all_vcd(void)
{
	for (auto &elem : elements) {
		if (elem->class_id() == "model_nest:") {
			auto *nest = static_cast<model_nest *>(elem.get());
			if (scene *sub = nest->get_scene())
				sub->unlock_all_vcd();
		}
		for (unsigned int pi = 0;; ++pi) {
			port *p = elem->port_at(pi);
			if (!p)
				break;
			for (const auto &w : p->get_wires()) {
				if (!w)
					continue;
				if (net *n = w->get_net())
					n->unlock_vcd();
				w->unlock_vcd();
			}
		}
	}
}

void scene::collect_vcd_names(std::vector<std::string> &out_names) const
{
	std::unordered_set<std::string> seen(out_names.begin(), out_names.end());
	for (const auto &elem : elements) {
		if (elem->class_id() == "model_nest:") {
			auto *nest = static_cast<model_nest *>(elem.get());
			if (const scene *sub = nest->get_scene()) {
				sub->collect_vcd_names(out_names);
				for (const auto &nm : out_names)
					seen.insert(nm);
			}
		}
		for (unsigned int pi = 0;; ++pi) {
			port *p = elem->port_at(pi);
			if (!p)
				break;
			for (const auto &w : p->get_wires()) {
				if (!w)
					continue;
				const std::string &vn = w->get_vcd_name();
				if (!vn.empty() && seen.insert(vn).second)
					out_names.push_back(vn);
			}
		}
	}
}

void scene::collect_vcd_qualifiers(
	std::unordered_map<std::string, std::pair<std::string, bool>> &out_qualifiers) const
{
	for (const auto &elem : elements) {
		if (elem->class_id() == "model_nest:") {
			auto *nest = static_cast<model_nest *>(elem.get());
			if (const scene *sub = nest->get_scene())
				sub->collect_vcd_qualifiers(out_qualifiers);
		}
		for (unsigned int pi = 0;; ++pi) {
			port *p = elem->port_at(pi);
			if (!p)
				break;
			for (const auto &w : p->get_wires()) {
				if (!w)
					continue;
				const std::string &vn = w->get_vcd_name();
				const std::string &qn = w->get_vcd_valid_name();
				if (!vn.empty() && !qn.empty() && !out_qualifiers.count(vn))
					out_qualifiers.emplace(vn, std::make_pair(qn, w->is_vcd_valid_active_low()));
			}
		}
	}
}
