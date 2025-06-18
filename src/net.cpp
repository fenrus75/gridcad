#include "gridcad.h"

#include "net.h"
#include "wire.h"
#include "port.h"

net::net(void)
{
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
}



void net::validate(void)
{
	int count = 0;
	/* verify there is only one PORT_OUT */
	for (auto port:ports)
		if (port->direction == PORT_OUT)
			count++;
	if (count != 1) {
		printf("NET IS INVALID, %i output ports\n", count);
		value.is_error = true;
	} else {
		value.is_error = false;
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

	set_value(newvalue, ttl);
}
