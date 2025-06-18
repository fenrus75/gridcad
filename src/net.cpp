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


