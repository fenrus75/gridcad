#pragma once

#include "gridcad.h"

#include "value.h"

class wire;
class port;

class net {
public:
	net(void);
	void destroy(void);   /* prepare the net object for delete */
	void add_port(class port *port);
	void add_wire(class wire *wire);
	void remove_port(class port *port);
	void remove_wire(class wire *wire);

	void validate(void);

	void set_value(class value *newval, int ttl);
	void update_value(class value *newval, int ttl);
	void apply_vcd_value(const struct value &v);
	void unlock_vcd(void);
	bool is_vcd_locked(void) const { return vcd_locked; };
	void sync_existing_value(void);
	void update_net_distances(void);

	void update_color(int color);
	unsigned int netnum = 0;
	
	std::string get_verilog_name(void);
	void set_special_verilog_name(std::string name) { special_verilog_name = name; };
	
	int get_width(void);
	std::string get_width_string(void);
	
	std::string get_verilog_wire_decl(void);
	
	bool is_empty(void);
	const std::vector<class wire *> &get_wires(void) const { return wires; };
	
protected:
	std::vector<class wire *> wires;
	std::vector<class port *> ports;

	bool has_z = false;
	bool vcd_locked = false;
	struct value value = {};
	int color = 0;
	
	std::string verilog_name = "";
	std::string special_verilog_name = "";
};

