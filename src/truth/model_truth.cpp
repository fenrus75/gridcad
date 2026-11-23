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
#include "model_truth.h"
#include "port.h"
#include "truthcanvas.h"
#include "contextmenu.h"

#include <sys/time.h>
#include <algorithm>

model_truth::model_truth(float _X, float _Y):element(1, 1, "Truth Table")
{
	unsigned int i;
	sizeX = 4;
	sizeY = 4;
	X = floorf(_X);
	Y = floorf(_Y);

	inputs = 2;
	outputs = 1;

	add_port(-1, 1, "In0", PORT_IN);
	add_port(-1, 2, "In1", PORT_IN);

	add_port(sizeX, 1, "Out1", PORT_OUT);

	values.resize((1 << inputs)); /* row 0 is the title bar/names */
	for (i = 0; i < values.size(); i++) {
		values[i].resize(inputs + outputs);
		for (unsigned int j = 0; j < inputs; j ++) {
			if ( (i >> j) & 1)
				values[i][inputs - j - 1] = '1';
			else
				values[i][inputs - j - 1] = '0';
		}
		for (unsigned int j = 0; j < outputs; j ++) {
			values[i][inputs + j] = '0';
		}
	}



	names.resize(inputs + outputs);
	for (i = 0; i < inputs; i++) {
		names[i] = std::format("In{}", i);
	}
	for (i = 0; i < outputs; i++) {
		names[inputs + i] = std::format("Out{}", i);
	}
	menu->add_item("Edit name", callback_editname);
	name_edit = std::make_unique<class name>(&name);
}

model_truth::~model_truth(void) = default;

void model_truth::drawAt(class basecanvas * canvas, float X, float Y, int type)
{
	if (!selected)
		name_edit->set_edit_mode(false);
	if (selected) {
		canvas->draw_image("assets/model_truth/truthtable_selected.png", X, Y, sizeX, sizeY, Alpha(type));
	} else {
		canvas->draw_image("assets/model_truth/truth_lt.png", X, Y, 1, 1, Alpha(type));
		canvas->draw_image("assets/model_truth/truth_rt.png", X + sizeX - 1, Y, 1, 1, Alpha(type));
		canvas->draw_image("assets/model_truth/truth_lb.png", X, Y + sizeY - 1, 1, 1, Alpha(type));
		canvas->draw_image("assets/model_truth/truth_rb.png", X + sizeX - 1, Y + sizeY - 1, 1, 1, Alpha(type));

		canvas->draw_image("assets/model_truth/truth_lm.png", X, Y + 1, 1, sizeY - 2, Alpha(type));
		canvas->draw_image("assets/model_truth/truth_rm.png", X + sizeX - 1, Y + 1, 1, sizeY - 2, Alpha(type));
		canvas->draw_image("assets/model_truth/truth_mt.png", X + 1, Y, sizeX - 2, 1, Alpha(type));
		canvas->draw_image("assets/model_truth/truth_mb.png", X + 1, Y + sizeY - 1, sizeX - 2, 1, Alpha(type));

		canvas->draw_image("assets/model_truth/truth_mm.png", X + 1, Y + 1, sizeX - 2, sizeY - 2, Alpha(type));
	}

	canvas->draw_image("assets/model_truth/truthtable_text.png", X + 0.2, Y + 0.2, sizeX - 0.4, sizeY - 0.4, Alpha(type), true);
	name_edit->drawAt(canvas,X, Y + sizeY, sizeX);

	hover_ports(canvas);

	for (auto &port : ports) {
		port->drawAt(canvas, X, Y, COLOR_WIRE_SOLID);
		port->draw_wires(canvas);
	}
}


bool model_truth::mouse_select(float _X, float _Y)
{
	long int click = time(nullptr);

	if (click - previous_click > 1) {
		previous_click = click;
		return false;
	}	

	if (!canvas) {
		logger::get().info("Spawning a new window");
		canvas = std::make_unique<class truthcanvas>(this);
		register_new_canvas(canvas.get());	
	} else {
		canvas->unhide();
	}
	previous_click = click;
	return false;
}

void model_truth::to_json(json & j)
{
	element::to_json(j);
	j["names"] = names;
	j["values"] = values;
	j["inputs"] = inputs;
	j["outputs"] = outputs;
	j["flip_input_ports"] = flip_input_ports;
	j["flip_output_ports"] = flip_output_ports;
	if (!input_port_widths.empty())
		j["input_port_widths"] = input_port_widths;
	if (!packed_lut_outputs.empty())
		j["packed_lut_outputs"] = packed_lut_outputs;
}

void model_truth::from_json(json & j)
{
	/* Restore LUT dimensions and packed metadata BEFORE element::from_json() attaches wires and triggers calculate(). */
	names = j["names"];
	values = j["values"];
	inputs = j["inputs"];
	outputs = j["outputs"];
	flip_input_ports = j.value("flip_input_ports", false);
	flip_output_ports = j.value("flip_output_ports", false);
	input_port_widths.clear();
	if (j.contains("input_port_widths") && j["input_port_widths"].is_array()) {
		for (const auto &v : j["input_port_widths"]) {
			if (v.is_number_integer())
				input_port_widths.push_back(std::clamp(v.get<int>(), 1, 64));
		}
	}
	packed_lut_outputs.clear();
	if (j.contains("packed_lut_outputs") && j["packed_lut_outputs"].is_array()) {
		for (const auto &v : j["packed_lut_outputs"]) {
			if (v.is_string())
				packed_lut_outputs.push_back(v.get<std::string>());
		}
	}

	element::from_json(j);
}

void model_truth::add_output(void)
{
	unsigned int y;

	for (y = 0; y < values.size(); y++)
		values[y].push_back('0');
	outputs++;
	std::string buf = std::format("Out{}", outputs - 1);
	names.push_back(buf);

	add_port(sizeX, outputs, buf, PORT_OUT, 1);

	sizeY = std::max(outputs + 2, std::max(inputs + 2, 4U));

}

void model_truth::del_output(void)
{
	unsigned int y;

	if (outputs <= 1)
		return;

	for (y = 0; y < values.size(); y++)
		values[y].pop_back();
	outputs--;
	names.pop_back();

	ports.back()->remove_wires();
	ports.pop_back();

}

static bool compare_line(std::vector<char> A, std::vector<char> B)
{
	unsigned int x;
	for (x = 0; x < A.size(); x++) {
		if (A[x] < B[x])
			return true;
		if (A[x] > B[x])
			return false;
	}

	return false;
}

static bool same_inputs(std::vector<char> A, std::vector<char> B, unsigned int inputs)
{
	unsigned int x;
	for (x = 0; x < inputs; x++) {
		if (A[x] != B[x])
			return false;
	}

	return true;
}


void model_truth::add_input(void)
{
	unsigned int y;

	logger::get().debug("Adding input");

	std::vector<std::vector<char>> values2;	

	values2 = values;

	std::string buf = std::format("In{}", inputs);
	names.insert(names.begin() + inputs , buf);
	for (y = 0; y < values.size(); y++) {
		values[y].insert(values[y].begin() + inputs , '0');
		values2[y].insert(values2[y].begin() + inputs , '1');
	}
	values.insert(values.end(), values2.begin(), values2.end());
	inputs++;

	std::sort(values.begin(), values.end(), compare_line);

	auto _port = std::make_unique<port>(buf, PORT_IN, 1);
	_port->X = -1;
	_port->Y = inputs;
	_port->parent = this;
	ports.insert(ports.begin() + inputs - 1, std::move(_port));

	sizeY = std::max(outputs + 2, std::max(inputs + 2, 4U));

}

void model_truth::del_input(void)
{
	unsigned int y;

	names.erase(names.begin() + inputs-1);
	for (y = 0; y < values.size(); y++) {
		values[y].erase(values[y].begin() + inputs -1);
	}

	ports[inputs - 1]->remove_wires();
	ports.erase(ports.begin() + inputs - 1);

	inputs--;

	/* we go backwards to avoid O(N^2) operations */	
	for (y = values.size() -1; y > 0; y--) {
		if (same_inputs(values[y], values[y-1], inputs)) {
			values.erase(values.begin() + y);
		}
	}


}

static int hdist(std::vector<char> A, std::vector<char> B)
{
	unsigned int x;
	int dist = 0;

	for (x = 0; x < A.size(); x++) {
		if (A[x]=='0' &&  B[x]=='1')
			dist++;
		if (A[x]=='1' &&  B[x]=='0')
			dist++;
	}

	return dist;
}
static int hdist_strict(std::vector<char> A, std::vector<char> B)
{
	unsigned int x;
	int dist = 0;

	for (x = 0; x < A.size(); x++) {
		if (A[x] !=  B[x])
			dist++;
	}

	return dist;
}


void model_truth::turn_to_X(unsigned int X, unsigned int Y)
{
	unsigned int y;

	if (values[Y][X] == 'X')
		return;


	values[Y][X] = 'X';

	/* at this point there is another line that has a hamming distance of 0 -- which we need to delete */
	for (y = 0; y < values.size(); y++) {
		if (y == Y)
			continue;
		if (hdist_strict(values[y], values[Y]) == 1) {
			values.erase(values.begin() + y);
			break;
		}
	}	
}
void model_truth::turn_from_X(unsigned int X, unsigned int Y)
{
	std::vector<char> line;

	if (values[Y][X] != 'X')
		return;

	values[Y][X] = '0';
	line = values[Y];
	line[X] = '1';
	values.push_back(line);
	std::sort(values.begin(), values.end(), compare_line);	
}

void model_truth::configure_lut(int num_inputs, std::string_view lut_bits)
{
	std::vector<std::string> single{std::string(lut_bits)};
	configure_lut(num_inputs, single);
}

void model_truth::configure_lut(int num_inputs, const std::vector<std::string> &lut_outputs)
{
	input_port_widths.clear();
	packed_lut_outputs.clear();

	const int target_inputs = std::clamp(num_inputs, 1, 8);
	const int target_outputs = std::clamp(static_cast<int>(lut_outputs.size()), 1, 64);

	while (static_cast<int>(inputs) < target_inputs)
		add_input();
	while (static_cast<int>(inputs) > target_inputs)
		del_input();

	while (static_cast<int>(outputs) < target_outputs)
		add_output();
	while (static_cast<int>(outputs) > target_outputs)
		del_output();

	for (unsigned int k = 0; k < inputs; ++k)
		names[k] = std::format("A{}", k);
	for (unsigned int o = 0; o < outputs; ++o)
		names[inputs + o] = (outputs == 1) ? "Y" : std::format("Y{}", o);
	names_to_ports();

	/*
	 * Yosys/Turbostar LUT strings are MSB-first: bit `idx` from the right
	 * gives the output when {A_{W-1}, ..., A_0} == idx.
	 */
	for (auto &row : values) {
		if (row.size() < inputs + outputs)
			continue;
		size_t lut_idx = 0;
		for (unsigned int k = 0; k < inputs; ++k) {
			if (row[k] == '1')
				lut_idx |= (size_t{1} << k);
		}
		for (unsigned int o = 0; o < outputs; ++o) {
			const std::string &bits = (o < lut_outputs.size()) ? lut_outputs[o] : std::string();
			char out_bit = '0';
			if (lut_idx < bits.size())
				out_bit = (bits[bits.size() - 1 - lut_idx] == '1') ? '1' : '0';
			row[inputs + o] = out_bit;
		}
	}
}

void model_truth::configure_lut_packed(const std::vector<int> &in_widths,
                                       const std::vector<std::string> &in_names,
                                       const std::vector<std::string> &lut_outputs)
{
	if (in_widths.empty())
		return;

	const int target_inputs = std::clamp(static_cast<int>(in_widths.size()), 1, 8);
	const int target_outputs = std::clamp(static_cast<int>(lut_outputs.size()), 1, 64);

	while (static_cast<int>(inputs) < target_inputs)
		add_input();
	while (static_cast<int>(inputs) > target_inputs)
		del_input();

	while (static_cast<int>(outputs) < target_outputs)
		add_output();
	while (static_cast<int>(outputs) > target_outputs)
		del_output();

	input_port_widths.clear();
	input_port_widths.reserve(inputs);
	for (unsigned int k = 0; k < inputs; ++k) {
		const int w = std::clamp(in_widths[k], 1, 8);
		input_port_widths.push_back(w);
		if (k < ports.size() && ports[k])
			ports[k]->set_width(w);
		names[k] = (k < in_names.size() && !in_names[k].empty())
		               ? in_names[k]
		               : std::format("A{}", k);
	}
	for (unsigned int o = 0; o < outputs; ++o)
		names[inputs + o] = (outputs == 1) ? "Y" : std::format("Y{}", o);

	packed_lut_outputs.assign(lut_outputs.begin(),
	                          lut_outputs.begin() + std::min<size_t>(lut_outputs.size(), outputs));
	names_to_ports();
	reseat();
}

void model_truth::calculate(int ttl)
{
	if (ttl < 1)
		return;

	if (!packed_lut_outputs.empty() && input_port_widths.size() == inputs) {
		size_t lut_idx = 0;
		int bit_pos = 0;
		constexpr int kSizeBits = static_cast<int>(sizeof(size_t) * 8);
		for (unsigned int k = 0; k < inputs; ++k) {
			const int w = input_port_widths[k];
			if (w <= 0)
				continue;
			const struct value &v = ports[k]->value;
			const uint64_t raw = (v.type == VALUE_TYPE_BOOL) ? (v.boolval ? 1ULL : 0ULL) : v.intval;
			const uint64_t mask = (w >= 64) ? ~uint64_t{0} : ((uint64_t{1} << w) - 1);
			if (bit_pos < kSizeBits)
				lut_idx |= static_cast<size_t>(raw & mask) << bit_pos;
			if (w >= kSizeBits - bit_pos)
				bit_pos = kSizeBits;
			else
				bit_pos += w;
		}
		for (unsigned int o = 0; o < outputs; ++o) {
			const std::string &bits = (o < packed_lut_outputs.size()) ? packed_lut_outputs[o] : std::string();
			const bool out_bit = (lut_idx < bits.size()) &&
			                     (bits[bits.size() - 1 - lut_idx] == '1');
			struct value out = {};
			out.valid = true;
			out.type = VALUE_TYPE_BOOL;
			out.boolval = out_bit;
			out.intval = out_bit ? 1 : 0;
			update_value_net(&out, inputs + o, ttl - 1);
		}
		return;
	}

	std::vector<char> inp;
	unsigned int x, y;

	inp.resize(inputs);
	for (x = 0; x < inputs; x++) {
		if (ports[x]->value.boolval)
			inp[x] = '1';
		else
			inp[x] = '0';
	}

	for (y = 0; y < values.size(); y++) {
		if (hdist(inp, values[y]) == 0) {
			for (x = 0; x < outputs; x++) {
				struct value value = {};
				value.valid = true;
				value.boolval = values[y][inputs + x] == '1';
				//				ports[inputs + x]->value.boolval = values[y][inputs + x] == '1';
				//ports[inputs + x]->value.valid = true;
				//				ports[inputs + x]->notify(ttl - 1);
				update_value_net(&value, inputs + x, ttl - 1);
			}
		}

	}
}


void model_truth::names_to_ports(void)
{
	unsigned int x;

	for (x = 0; x < inputs; x++) {
		if (!flip_input_ports)
			ports[x]->update_name(names[x]);
		else
			ports[inputs - x - 1]->update_name(names[x]);
	}
	for (x = inputs; x < names.size(); x++) {
		if (!flip_output_ports)
			ports[x]->update_name(names[x]);
		else
			ports[inputs + outputs -1 -x]->update_name(names[x]);
	}
}

void model_truth::handle_event(class basecanvas *canvas, SDL_Event &event)
{
	element::handle_event(canvas, event);
	switch (event.type) {
		case SDL_MOUSEMOTION:
			mouseX = canvas->scr_to_X(event.motion.x); 
			mouseY = canvas->scr_to_Y(event.motion.y);

			break;
	}
	if (!selected || !single)
		return;
	switch (event.type) {
		case SDL_KEYDOWN:        
			switch (event.key.keysym.sym) {
				case SDLK_RETURN:
					name_edit->toggle_edit_mode();
					break;
			}
			break;
	}
	name_edit->handle_event(event);
}


std::string model_truth::get_verilog_main(void)
{
	std::string s = "";
	if (verilog_module_name == "")
		verilog_module_name = append_random_bits(verilog_name + "_tt_");

	s = s + verilog_module_name + " " + get_verilog_name() + "(";
	bool first = true;
	for (auto &port : ports) {

		if (!port->has_net())
			continue;

		if (!first)
			s = s + ", ";
		first = false;
		s = s + "." + port->get_verilog_name() + "(";
		s = s + port->get_net_verilog_name();
		s = s + ")";

	}
	s = s + ");\n";

	return s;
}

/* TODO -- rather than a pure basic SOP, run espresso on this and make it more minimal */
std::string model_truth::get_verilog_modules(std::string path)
{
	std::string s = "";

	s = s + "module " + verilog_module_name + "\n (";
	bool first = true;
	for (auto &port : ports) {

		if (!first)
			s = s + ", ";
		first = false;
		if (port->direction == PORT_IN)
			s = s + "input ";
		if (port->direction == PORT_OUT)
			s = s + "output ";
		s = s +port->get_verilog_name();

	}
	s = s + ");\n\n";

	for (unsigned int i = inputs; i < names.size(); i++) {
		first = true;
		s = s +"assign " + names[i] + " = ";

		for (unsigned int Y = 0; Y < values.size(); Y++) {
			if (values[Y][i] == '1') {
				if (!first) 
					s = s + " | ";
				first = false;
				s = s + "(";
				bool nestedfirst = true;
				for (unsigned int x = 0; x < inputs; x++) {
					if (values[Y][x] == '1') {
						if (!nestedfirst)
							s = s + " & ";
						nestedfirst = false;
						s = s + names[x];
					}
					if (values[Y][x] == '0') {
						if (!nestedfirst)
							s = s + " & ";
						nestedfirst = false;
						s = s + "(!" + names[x] + ")";
					}
				}

				s = s + ")";
			}
		}

		if (first) 
			s = s + "1'b0";

		s = s + ";\n";
	}

	s = s + "endmodule\n";


	return s;
}


std::string model_truth::get_verilog_name(void)
{
	if (verilog_name == "") {
		verilog_name = name; 
		std::replace(verilog_name.begin(), verilog_name.end(), ' ', '_');
		std::replace(verilog_name.begin(), verilog_name.end(), '-', '_');
		std::replace(verilog_name.begin(), verilog_name.end(), '+', '_');
		verilog_name = append_random_bits(verilog_name + "_");
	}


	return verilog_name;
}

