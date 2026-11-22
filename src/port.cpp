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
#include "wire.h"
#include "port.h"
#include "net.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <sys/time.h>

void callback_set_color(class port *port, int color)
{
	logger::get().debug("Pushing color {}", color);
	port->color = color;
	port->push_net_color(color);
}

void port::add_menu(void)
{
	if (direction == PORT_OUT) {
		menu = std::make_unique<class port_contextmenu>(this);	
		menu->add_item(wire_color_name(0), 0, callback_set_color);
		menu->add_item(wire_color_name(1), 1, callback_set_color);
		menu->add_item(wire_color_name(2), 2, callback_set_color);
		menu->add_item(wire_color_name(3), 3, callback_set_color);
		menu->add_item(wire_color_name(4), 4, callback_set_color);
		menu->add_item(wire_color_name(5), 5, callback_set_color);
		menu->add_item(wire_color_name(6), 6, callback_set_color);
		menu->add_item(wire_color_name(7), 7, callback_set_color);
	};
}

port::port(std::string _name, int _direction, int _bus_width)
{

	direction = _direction;
	name = _name;
	bus_width = _bus_width;

	add_menu();
}

port::~port()
{
	while (wires.size() > 0) {
		auto wire = wires[0];
		if (class net *n = wire->get_net())
			n->remove_port(this);
		wire->del_port(this);
		remove_wire(wire.get());
	}
}


void port::add_wire(std::shared_ptr<class wire> wire)
{
	for (auto &_wire : wires) {
		if (wire == _wire)
			return;
	}

	if (bus_width == 0 && wire->get_width() > 0) {
		bus_width = wire->get_width();
		if (bus_width > 1)
			value.type = VALUE_TYPE_INT;
	}


	/* Only allow one input wire */
	if (wires.size() > 0 && direction == PORT_IN)
		return;

	if (wire->get_width() == 0) 
		wire->set_width( get_width());

	wires.push_back(wire);

	if (direction == PORT_OUT) {
		wire->update_value_final(&value, DEFAULT_TTL);
		wire->update_value_net(&value, DEFAULT_TTL);
		distance_from_outport = 0;
		wire->set_distance_from_outport(0);
	}
	wire->add_port(this);

	if (bus_width) {
		wire->set_width(bus_width);
	}

	if (direction == PORT_IN) {
		update_value_final(&(wire->value), DEFAULT_TTL);
	}
	if (direction == PORT_OUT)
		wire->push_wire_color(color);
	if (parent)
		parent->notify(DEFAULT_TTL);
	wire->notify(DEFAULT_TTL);
}
#if 0
void port::update_value(struct value *newvalue, int ttl)
{
	unsigned int best_dist = INT_MAX;

	if (bus_width == 0)
		for (auto &wire : wires) {
			if (wire->get_width() > bus_width)
				bus_width = wire->get_width();

		}


	if (direction == PORT_OUT) {
		distance_from_outport = 0;
		for (auto &wire : wires)
			wire->set_distance_from_outport(0);
	} else {
		for (auto &wire : wires)
			if (wire->get_distance_from_outport() < best_dist)
				best_dist = wire->get_distance_from_outport();

		for (auto &wire : wires) {
			distance_from_outport = best_dist + 1;
			wire->set_distance_from_outport(best_dist + 1);
		}
	}



	if (ttl <= 0)
		return;
	if (!newvalue->valid)
		return;
	if (value == *newvalue)
		return;

	value = *newvalue;	

	for (auto &wire : wires) {
		wire->update_value(newvalue, ttl -1);
	}

	//	notify(ttl -1);
	if (parent && direction != PORT_OUT)
		parent->notify(ttl -1);

}
#endif

bool port::update_distances(void)
{
	unsigned int best_dist = INT_MAX - 50;
	int changed = 0;

	if (direction == PORT_OUT) {
		distance_from_outport = 0;
		for (auto &wire : wires)
			changed += wire->set_distance_from_outport(0);
	} else if (direction == PORT_Z) {
		distance_from_outport = 1000;
		for (auto &wire : wires)
			changed += wire->set_distance_from_outport(1000);
	} else {
		for (auto &wire : wires) {
			if (wire->get_distance_from_outport() < best_dist) {
				best_dist = wire->get_distance_from_outport();
			}
		}

		for (auto &wire : wires) {
			if (distance_from_outport != best_dist + 1)
				changed++;
			distance_from_outport = best_dist + 1;
			changed += wire->set_distance_from_outport(best_dist + 1);
		}
	}
	return changed > 0;
}		


void port::update_value_final(struct value *newvalue, int ttl)
{
	unsigned int best_dist = INT_MAX;

	if (bus_width == 0)
		for (auto &wire : wires) {
			if (wire->get_width() > bus_width)
				bus_width = wire->get_width();

		}


	if (direction == PORT_OUT) {
		distance_from_outport = 0;
		for (auto &wire : wires)
			wire->set_distance_from_outport(0);
	} else {
		for (auto &wire : wires)
			if (wire->get_distance_from_outport() < best_dist)
				best_dist = wire->get_distance_from_outport();

		for (auto &wire : wires) {
			distance_from_outport = best_dist + 1;
			wire->set_distance_from_outport(best_dist + 1);
		}
	}



	value = *newvalue;	

	//	notify(ttl -1);
	if (parent && direction != PORT_OUT)
		parent->notify(ttl -1);

}

void port::drawAt(class basecanvas * canvas, float _X, float _Y, int type)
{
	//        if (type != DRAW_GHOST && type != DRAW_DND)

	draw_wires(canvas);
	if (direction == PORT_IN) {
		drawConnector(canvas, _X, _Y, X, Y, COLOR_ELEMENT_CONNECTOR + type);
	} else { 
		drawConnector(canvas, _X, _Y, X, Y, value_color(&value));
	}
}

void port::drawAt2(class basecanvas * canvas, float _X, float _Y, int type)
{
	if (direction == PORT_IN) {
		drawConnector(canvas, _X, _Y, X, Y, COLOR_ELEMENT_CONNECTOR + type);
	} else { 
		drawConnector(canvas, _X, _Y, X, Y, value_color(&value));
	}
}

void port::draw_wires(class basecanvas * canvas)
{
	for (auto &wire : wires) {
		wire->draw(canvas);
	}
}

void port::draw(class basecanvas *canvas, int color)
{
	drawAt(canvas, screenX, screenY, color);
}

void port::drawDistress(class basecanvas *canvas, float X, float Y)
{
	double radius = 1.0;
	struct timeval tv;
	double sin45 = 0.707;

	X = X + 0.5;
	Y = Y + 0.5;

	gettimeofday(&tv, nullptr);

	if (tv.tv_usec > 500000)
		tv.tv_usec = 1000000 - tv.tv_usec;

	radius = 0.1 + 0.5 * (tv.tv_usec / 500000.0);

	canvas->draw_line(X, Y - radius, X + radius * sin45, Y - radius * sin45, COLOR_VALUE_RED);
	canvas->draw_line(X + radius * sin45, Y - radius * sin45, X + radius, Y, COLOR_VALUE_RED);
	canvas->draw_line(X + radius, Y, X + radius * sin45, Y + radius * sin45, COLOR_VALUE_RED);
	canvas->draw_line(X + radius * sin45, Y + radius * sin45, X, Y + radius, COLOR_VALUE_RED);

	canvas->draw_line(X, Y - radius, X - radius * sin45, Y - radius * sin45, COLOR_VALUE_RED);
	canvas->draw_line(X - radius * sin45, Y - radius * sin45, X - radius, Y, COLOR_VALUE_RED);
	canvas->draw_line(X - radius, Y, X - radius * sin45, Y + radius * sin45, COLOR_VALUE_RED);
	canvas->draw_line(X - radius * sin45, Y + radius * sin45, X, Y + radius, COLOR_VALUE_RED);

}


extern bool wire_debug_mode;

void port::drawConnector(class basecanvas * canvas, float X, float Y, int cX, int cY, int type)
{
	std::string icon = "";
	if (direction == PORT_IN) {
		if (wires.size() == 0)
			icon = "assets/port_in_open.png";
		else
			icon = "assets/port_in_connected.png";
	} else	if (direction == PORT_Z) {
		icon = "assets/port_out_z.png";
	} else {
		if (value.boolval)
			icon = "assets/port_out_green.png";
		else
			icon = "assets/port_out_red.png";

		if (value.type == VALUE_TYPE_INT || bus_width > 1)
			icon = "assets/port_out_bus.png";
	}
	canvas->draw_image(icon, cX + X, cY + Y, 1, 1, Alpha(type));

	if (bus_width <= 1 && !wire_debug_mode) {
		if (name != "")
			canvas->draw_text(name, cX + X, cY + Y + 0.35, 1, 0.3);
	} else  {
		std::string s;
		if (wire_debug_mode)
			s = std::format("{}", distance_from_outport);
		else
			s = std::format("{}", bus_width);
		canvas->draw_text(s, cX + X, cY + Y + 0.15, 1, 0.55);
	}
	if (distress)
		drawDistress(canvas, X + cX, Y + cY);
}

void port::stop_drag(class basecanvas *canvas)
{
	update_distances();

	for (auto &wire : wires) {
		wire->reseat();
//		wire->route(canvas->get_scene());
		wire->clear_route();
	}

}

void port::notify(int ttl)
{	
}

std::shared_ptr<class wire> port::is_wire(float X, float Y)
{
	for (auto &wire : wires) {
		if (wire->intersect(X, Y))
			return wire;	
	}
	return nullptr;
}

void port::remove_wire(class wire *_wire)
{
	for (unsigned int i = 0; i< wires.size(); ++i) {
		if (wires[i].get() == _wire) {
			wires.erase(wires.begin() + i);
			return;
		}
	}
}
bool port::replace_wire(class wire *from, std::shared_ptr<class wire> to)
{
	bool result = true;
	remove_wire(from);
	add_wire(std::move(to));	
	return result;
}


void port::to_json(json &j)
{
	unsigned int i;
	j["X"] = X;
	j["Y"] = Y;
	j["screenX"] = screenX;
	j["screenY"] = screenY;
	j["name"] = name;
	j["value"] = value;
	j["direction"] = direction;
	j["bus_width"] = bus_width;
	j["linked_uuid"] = linked_uuid;
	j["color"] = color;
	j["verilog_name"] = verilog_name;

	j["wires"] = json::array();
	j["distance_from_outport"] = distance_from_outport;
	j["distress"] = distress;
	j["allow_fancy_verilog_name"] = allow_fancy_verilog_name;
	for (i = 0; i < wires.size(); i++) {
		json p;
		wires[i]->to_json(p);
		j["wires"][i] = p;
	}
}

void port::from_json(json &j)
{
	unsigned int i;
	X = j["X"];
	Y = j["Y"];
	screenX = j["screenX"];
	screenY = j["screenY"];
	name = j["name"];
	value = j["value"];
	direction = j["direction"];
	bus_width = j.value("bus_width", 1);
	linked_uuid = j.value("linked_uuid", "");
	verilog_name = j.value("verilog_name", name);
	color = j.value("color", 0);
	distress = j.value("distress", false);
	distance_from_outport = j.value("distance_from_outport", INT_MAX);
	allow_fancy_verilog_name = j.value("allow_fancy_verilog_name", false);
	for (i = 0; i < j["wires"].size(); i++) {
		auto wire = json_wire_factory(j["wires"][i]);
		const int saved_color = wire->get_color();
		add_wire(wire);
		wire->set_color(saved_color);
		wire->add_port(this);
		wire->reseat();
	}
	add_menu();
}

void port::remove_wires(void)
{
	while (wires.size() > 0) {
		auto wire = wires[0];
		wire->remove();
	}
	wires.clear();
}

void port::fill_grid(class wiregrid *grid)
{
	for (auto &wire : wires) {
		wire->fill_grid(grid);
	}
}

void port::remove_orphans(void)
{
	auto wires_copy = wires;
	for (auto &wire : wires_copy) {
		wire->remove_if_orphan();
	}
}

void port::update_name(std::string newname)
{
	name = newname;
}

void port::link_uuid(std::string _uuid)
{
	linked_uuid = _uuid;
}

void port::route_wires(void)
{
	for (auto &wire : wires) {
		wire->reseat();
		wire->redo_wires();
	}
}

int port::get_width(void)
{
	int width = 0;
	if (bus_width)
		return bus_width;
	for (auto &wire : wires) {
		width = std::max(width, wire->get_width());
	}
	return width;
}

void port::unsplice(void)
{
	if (wires.size() != 2)
		return;
	auto w1 = wires[0];
	auto w2 = wires[1];
	class port *p1, *p2;
	class net *existing_net = w1->get_net() ? w1->get_net() : w2->get_net();

	auto w3 = std::make_shared<class wire>(0,0,0,0);
	w3->name = !w1->name.empty() ? w1->name : w2->name;
	if (!w1->get_vcd_name().empty())
		w3->set_vcd_name(w1->get_vcd_name());
	else if (!w2->get_vcd_name().empty())
		w3->set_vcd_name(w2->get_vcd_name());
	if (!w1->get_vcd_valid_name().empty())
		w3->set_vcd_valid_signal(w1->get_vcd_valid_name(), w1->is_vcd_valid_active_low());
	else if (!w2->get_vcd_valid_name().empty())
		w3->set_vcd_valid_signal(w2->get_vcd_valid_name(), w2->is_vcd_valid_active_low());
	if (w1->get_label_table())
		w3->set_label_table(w1->get_label_table());
	else if (w2->get_label_table())
		w3->set_label_table(w2->get_label_table());
	if (!w1->get_src_location().empty() || !w1->get_src_verilog().empty())
		w3->set_src_info(w1->get_src_location(), w1->get_src_verilog());
	else if (!w2->get_src_location().empty() || !w2->get_src_verilog().empty())
		w3->set_src_info(w2->get_src_location(), w2->get_src_verilog());
	if (w1->is_critical_path())
		w3->set_timing_info(w1->get_timing_info());
	else if (w2->is_critical_path())
		w3->set_timing_info(w2->get_timing_info());
	if (w1->value.is_clock || w2->value.is_clock) {
		w3->value.valid = true;
		w3->value.is_clock = true;
	}
	w3->set_width(std::max(w1->get_width(), w2->get_width()));

	p1 = w1->get_other_port(this);
	p2 = w2->get_other_port(this);
	w1->remove();
	w2->remove();

	if (p1) {
		w3->add_port(p1);
		p1->add_wire(w3);
	} else {
		logger::get().warn("NO P1");
	}
	if (p2) {
		w3->add_port(p2);
		p2->add_wire(w3);
	} else {
		logger::get().warn("NO P3");
	}
	if (existing_net)
		w3->add_net(existing_net);
	w3->push_wire_color(w1->get_color());
	w3->reseat();

}


void port::push_wire_color(int color)
{
	for (auto &wire : wires)
		wire->push_wire_color(color);
}

void port::push_net_color(int color)
{
	if (wires.size() < 1)
		return;
	class net *net = wires[0]->get_net();

	if (net)
		net->update_color(color);
}




void port::cycle_color(void)
{
	for (auto &wire : wires)
		wire->clear_distance_from_outport();

	if (direction != PORT_OUT)
		return;

	for (auto &wire : wires)
		wire->set_distance_from_outport(0);

	if (value.is_clock)
		return;

	if (color != 0)
		push_wire_color(0);
	else
		push_wire_color(1);

	push_wire_color(color);

}

void port::reroute_all_wires(void)
{
	for (auto &wire : wires)
		wire->redo_wires();
}

void port::collect_nets(std::vector<std::string> *netmap)
{
	std::string s = get_net_verilog_wire_decl();
	for (auto net : *netmap) {
		if (net == s)
			return;
	}
	netmap->push_back(s);
}

std::string port::get_verilog_name(void)
{
	verilog_name = name;
	std::replace(verilog_name.begin(), verilog_name.end(), '-', '_');
	std::replace(verilog_name.begin(), verilog_name.end(), '+', '_');
	return verilog_name;
}

std::string port::get_tooltip(void)
{
	std::string s;
	if (value.type == VALUE_TYPE_INT)
		s = std::format("Name {}:{} value 0x{:x}", name, bus_width, value.intval);
	else
		s = std::format("Name {}:{} value {}", name, bus_width, value.boolval ? 1 : 0);
	if (direction == PORT_IN)
		s = "INPUT [" + s + "]";
	if (direction == PORT_OUT)
		s = "OUTPUT [" + s + "]";
	if (direction == PORT_Z)
		s = "TRISTATE [" + s + "]";
	return s;
}

void port::add_net(class net *net)
{
	for (auto &wire : wires)
		wire->add_net(net);

	net->add_port(this);
	if (allow_fancy_verilog_name)
		net->set_special_verilog_name(name);
}

void port::remove_net(void)
{
	for (auto &wire : wires)
		wire->remove_net();
}

bool port::has_net(void)
{
	for (auto &wire : wires)
		if (wire->has_net())
			return true;
	return false;
}

class net * port::get_net(void)
{
	for (auto &wire : wires)
		if (wire->has_net())
			return wire->get_net();
	return nullptr;
}
void port::update_value_net(struct value *newvalue, int ttl)
{
	if (wires.size() < 1)
		return;
	wires[0]->update_value_net(newvalue, ttl);

}

void port::check_reverse(void)
{
	for (auto &wire : wires)
		wire->check_reverse();
}

std::string port::get_net_verilog_name(std::string_view fallback)
{
	class net *net;
	if (wires.size() < 1)
		return std::string(fallback);
	net = wires[0]->get_net();
	return net->get_verilog_name();
}

std::string port::get_net_verilog_wire_decl(void)
{
	class net *net;
	if (wires.size() < 1)
		return "";
	net = wires[0]->get_net();
	return net->get_verilog_wire_decl();
}

int port::get_net_width(void)
{
	class net *net;
	if (wires.size() < 1)
		return 0;
	net = wires[0]->get_net();
	if (net)
		return net->get_width();
	else 
		return get_width();
}

void port::validate(void)
{
	logger::get().debug("Validating port {}", name);
	distress = false;
	if (name == "clk" && wires.size() == 0 && direction == PORT_IN)
		distress = true;

	if (wires.size() < 1)
		return;

	class net *net = wires[0]->get_net();
	if (net) {
		int nw = net->get_width();
		if (bus_width > 0 && nw != bus_width)
			distress = true;
	}
}


std::string port::get_verilog_width(void)
{
    std::string s = "";
    unsigned int w = get_width();
    
    if (w > 1)
      s = "[" + std::to_string(w-1) + ":0]";
    return s;
}

void port::update_fancy_name(std::string_view newname)
{
	class net *net;
	net = get_net();
	if (!net)
		return;
	if (allow_fancy_verilog_name)
		net->set_special_verilog_name(std::string(newname));
}

void port::clear_distress(void) 
{ 
	bool old_distress = distress;
	distress = false;
	if (parent && old_distress)
		parent->notify(DEFAULT_TTL); 
};

std::string port::get_label(uint64_t val) const
{
	for (const auto &w : wires) {
		if (w) {
			std::string lbl = w->get_label(val);
			if (!lbl.empty())
				return lbl;
		}
	}
	return "";
}

bool port::enum_display_phase(void)
{
	return ((SDL_GetTicks64() / 1000ULL) & 1ULL) == 0;
}
