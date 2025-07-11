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

	void validate(void);

	void set_value(class value *newval, int ttl);
	void update_value(class value *newval, int ttl);
	void update_net_distances(void);

	void update_color(int color);
	unsigned int netnum;
	
	std::string get_verilog_name(void);
	void set_special_verilog_name(std::string name) { special_verilog_name = name; };
	
	int get_width(void);
	std::string get_width_string(void);
	
	std::string get_verilog_wire_decl(void);
	
	bool is_empty(void);
	
protected:
	std::vector<class wire *> wires;
	std::vector<class port *> ports;

	bool has_z = false;
	struct value value = {};
	int color = 0;
	
	std::string verilog_name = "";
	std::string special_verilog_name = "";
};

