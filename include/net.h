#pragma once

#include "gridcad.h"

#include "value.h"

class wire;
class port;

class net {
public:
	net(void);
	~net(void);

	void destroy(void);   /* prepare the net object for delete */
	void add_port(class port *port);
	void add_wire(class wire *wire);
	void remove_port(class port *port);
	void remove_wire(class wire *wire);

protected:
	std::vector<class wire *> wires;
	std::vector<class port *> ports;

	struct value value;
};

