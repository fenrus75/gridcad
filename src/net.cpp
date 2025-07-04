#include "gridcad.h"

#include "net.h"
#include "wire.h"
#include "port.h"

unsigned int global_netnum = 0;

net::net(void)
{
	netnum = global_netnum++;
}

net::~net(void)
{
}

void net::add_port(class port *port)
{
	for (auto p : ports)
		if (p == port)
			return;

	ports.push_back(port);

	port->add_net(this);
}

void net::add_wire(class wire *wire)
{
	for (auto w : wires)
		if (w == wire)
			return;
	wires.push_back(wire);
	wire->add_net(this);
}

void net::remove_wire(class wire *wire)
{
	for (unsigned int i = 0; i < wires.size(); i++) {
		if (wire == wires[i]) {
			wires.erase(wires.begin() + i);
			return;
		}
	}
}

void net::remove_port(class port *port)
{
	for (unsigned int i = 0; i < ports.size(); i++) {
		if (port== ports[i]) {
			ports.erase(ports.begin() + i);
			return;
		}
	}
}

void net::destroy(void)
{
	while (wires.size() > 0)
		wires[0]->remove_net();

}


void net::update_net_distances(void)
{
	bool changed = true;
	/* step 1: set all to high */

	for (auto wire:wires)
		wire->clear_distance_from_outport();
	/* step 2: set OUT to 0 */
	for (auto port : ports)
		port->update_distances();
	for (auto wire : wires)
		wire->update_distances();
	/* step 3: propagate until no more changes */
	while (changed) {
		changed = false;
		for (auto port : ports)
			changed |= port->update_distances();
		for (auto wire : wires)
			changed |= wire->update_distances();
	}

	for (auto wire: wires)
		wire->check_reverse();
}


void net::validate(void)
{
	int count = 0;
	int zcount = 0;
	/* verify there is only one PORT_OUT */
	for (auto port:ports)
		if (port->direction == PORT_OUT)
			count++;
	for (auto port:ports)
		if (port->direction == PORT_Z)
			zcount++;
	if (count != 1) {
		//		printf("NET IS INVALID, %i output ports\n", count);
		if (!value.is_error && count > 1) {
			value.is_error = true;
			set_value(&value, DEFAULT_TTL);
		}
		for (auto port:ports)
			if (port->direction == PORT_OUT)
				port->set_distress();
			else
				port->clear_distress();
	} else {
		if (value.is_error) {
			value.is_error = false;
			set_value(&value, DEFAULT_TTL);
		}
		for (auto port:ports)
			port->clear_distress();
	}
	if (zcount > 0)
		has_z = true;
	else
		has_z = false;

	if (has_z && count == 0) {
		for (auto wire:wires)
			wire->set_is_z(true);
	} else {
		for (auto wire:wires)
			wire->set_is_z(false);
	}
}

void net::set_value(struct value *newval, int ttl)
{
	for (auto port:ports) {
		port->update_value_final(newval, ttl - 1);
	}
	for (auto wire:wires) {
		wire->update_value_final(newval, ttl - 1);
	}
}

void net::update_value(struct value *newvalue, int ttl)
{
	if (memcmp(&value, newvalue, sizeof(struct value)) == 0)
		return;

	value = *newvalue;
	set_value(newvalue, ttl);
	update_net_distances();
}


void net::update_color(int _color)
{
	color = _color;
	for (auto wire:wires) 
		wire->set_color(_color);
}

std::string net::get_verilog_name(void)
{
	if (special_verilog_name != "")
		verilog_name = special_verilog_name;
		
	if (verilog_name == "")
		verilog_name = generate_wire_name();

	std::replace(verilog_name.begin(), verilog_name.end(), '-', '_');
	std::replace(verilog_name.begin(), verilog_name.end(), ' ', '_');
	std::replace(verilog_name.begin(), verilog_name.end(), '+', '_');

	return verilog_name;
}

int net::get_width(void)
{
	int w = 1;
	for (auto wire : wires) {
		if (wire->get_width() > w)
			w = wire->get_width();
	}
	for (auto port : ports) {
		if (port->get_width() > w)
			w = port->get_width();
	}
	return w;
}

std::string net::get_width_string(void)
{
	std::string s = "";
	int w = get_width();
	if (w < 2)
		return "";
	s = s + "[" + std::to_string(w -1) + ":0]";
	return s;
}


std::string net::get_verilog_wire_decl(void)
{
	std::string s = "";
	/* special verilog names already have their own verilog decleration */
	if (special_verilog_name == "")
		s = "wire " + get_width_string() + " " + get_verilog_name() + ";";
	return s;
}

bool net::is_empty(void)
{
	if (ports.size() > 0)
		return false;
	if (wires.size() > 0)
		return false;
	return true;
}
