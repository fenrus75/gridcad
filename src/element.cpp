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

#include <assert.h>
#include <math.h>

#include "gridcad.h"
#include "scene.h"
#include "element.h"
#include "port.h"
#include "contextmenu.h"
#include "wire.h"
#include "net.h"

static void callback_delete(class element *element)
{
	element->delete_element();
}

static void callback_select(class element *element)
{
	element->select();
}

void callback_editname(class element *element)
{
	element->select();
	element->enter_edit_mode();
}

void callback_rotate(class element *element)
{
	element->rotate_ports();
}

element::element(int _sizeX, int _sizeY, std::string _name, std::string _parent)
{
	sizeX = _sizeX;
	sizeY = _sizeY;
	place(0,0);
	name = _name;
	parental_name = _parent;
	uuid = generate_semi_uuid();
	menu = new contextmenu(this);
	menu->add_item("Delete", callback_delete);
	menu->add_item("Select", callback_select);
	menu->add_item("Rotate", callback_rotate);

}

element::~element()
{
	//    printf("Destructor for element %s\n", name.c_str());
	remove_nets();
	for (auto port : ports) {
		delete port;
	}
	ports.clear();
	free_nets_memory();
	delete menu;
}

void element::place(int _X, int _Y)
{
	X = _X; Xghost = _X; Xdnd = _X;
	Y = _Y; Yghost = _Y; Ydnd = _Y;
}


void element::drawAt(class basecanvas *canvas, float X, float Y, int type)
{
	assert (type <= DRAW_ORIGIN);

	canvas->draw_box(X + 1, Y + 1, X + sizeX , Y + sizeY , COLOR_ELEMENT_NORMAL + type);
	if (type == DRAW_NORMAL || type == DRAW_ORIGIN)
		canvas->draw_box(X + 1.2, Y + 1.2, X + sizeX - 0.2, Y + sizeY - 0.2, COLOR_ELEMENT_INSIDE);

	for (auto port: ports) {
		port->drawAt(canvas, X, Y, type);
	}
}


void element::draw(class basecanvas *canvas, int type)
{
	switch (type) {
		case DRAW_NORMAL:
			drawAt(canvas, X, Y, DRAW_NORMAL);
			break;
		case DRAW_GHOST:
			drawAt(canvas, Xghost, Yghost, DRAW_GHOST);
			break;
		case DRAW_DND:
			drawAt(canvas, Xdnd, Ydnd, DRAW_DND);
			break;
		case DRAW_ORIGIN:
			drawAt(canvas, X, Y, DRAW_ORIGIN);
			break;

		default:
			assert(0);

	}
}

void element::draw_phase2(class basecanvas *canvas, int type)
{
	switch (type) {
		case DRAW_NORMAL:
			for (auto port:ports)
				port->drawAt2(canvas, X, Y, type);
			break;

		default:
			{}        
	}
}

void element::start_drag(float _X, float _Y)
{
	over_drag_threshold = false;
	Xghost = X;
	Yghost = Y;
	Xdnd = X;
	Ydnd = Y;
	X_in_drag = _X - X;
	Y_in_drag = _Y - Y;
}

void element::update_drag(class basecanvas *canvas, class scene *scene, float _X, float _Y)
{
	if (fabsf(_X - X) > 1 || fabsf(_Y - Y) > 1)
		over_drag_threshold = true;


	Xdnd = _X - X_in_drag;
	Ydnd = _Y - Y_in_drag;

	if (over_drag_threshold) {
		float _Xghost, _Yghost;
		_Xghost = floorf(_X - X_in_drag + 0.25);
		_Yghost = floorf(_Y - Y_in_drag + 0.25);
		if (scene->can_place_element(_Xghost, _Yghost, sizeX, sizeY, this)) {
			Xghost = _Xghost;
			Yghost = _Yghost;
		} 
	}
}

bool element::stop_drag(class basecanvas *canvas)
{
	bool no_move = false;
	/* we did not move at all */

	printf("%5.2f %5.2f      %5.2f  %5.2f\n", X,Y,Xghost, Yghost); 
	if (X == Xghost && Y == Yghost) {
		no_move = true;
	}
	X = Xghost;
	Y = Yghost;

	reseat();
	over_drag_threshold = false;

	if (!no_move)
		for (auto port : ports)
			port->stop_drag(canvas);

	calculate(DEFAULT_TTL);
	return !no_move;
}


/* does a mouse press hit this specific element */
bool element::intersect(float _X, float _Y)
{
	int mX = _X, mY = _Y;
	if (mX >= X && mY >= Y  && mX < X + sizeX && mY < Y + sizeY)
		return true;
	return false;
}

bool element::intersect_full(float _X, float _Y)
{
	bool ret = false;
	int mX = _X, mY = _Y;
	ret = intersect(_X, _Y);
	if (ret) {
		return ret;
	}

	for (auto port : ports) { 
		if (port->screenX == mX && port->screenY == mY) {
			return true;
		}
	}
	return false;
}

bool element::intersect_float(float _X, float _Y)
{
	int Xd, Yd;
	Xd = floorf(Xdnd + 0.25);
	Yd = floorf(Ydnd + 0.25);

	int mX, mY;
	mX = _X;
	mY = _Y;
	if (mX >= Xd && mY >= Yd  && mX < Xd + sizeX && mY < Yd + sizeY) {
		return true;
	}

	for (auto port : ports) {
		if (port->X + Xd == mX && port->Y + Yd == mY) {
			return true;
		}
	}
	return false;
}

void element::add_port(int X, int Y, const char *_name, int direction, int _width)
{
	class port *_port;
	_port = new port(_name, direction);
	_port->X = X;
	_port->Y = Y;
	_port->parent = this;
	_port->set_width(_width);
	ports.push_back(_port);
}

class port * element::is_port(float _X, float _Y)
{
	/* make relative to the element */
	_X -= X;
	_Y -= Y;

	for (auto port : ports) {
		if (floorf(_X) == port->X && floorf(_Y) == port->Y)
			return port;
	}
	return NULL;
}

void element::fill_grid(class wiregrid *grid)
{
	int x,y;

	for (x = 0; x < sizeX; x++)
		for (y = 0; y < sizeY; y++)
			grid->block_point(X+x, Y+y);

	for (x = -1; x < sizeX + 1; x++)
		for (y = -1; y < sizeY +1; y++)
			grid->add_soft_cost(X+x, Y+y, 0.6);

	for (auto port: ports) {
		grid->block_point(X + port->X, Y + port->Y);
		grid->add_soft_cost(X + port->X + 1, Y + port->Y + 0, 0.3);
		grid->add_soft_cost(X + port->X + 1, Y + port->Y + 1, 0.6);
		grid->add_soft_cost(X + port->X + 1, Y + port->Y - 1, 0.6);
		grid->add_soft_cost(X + port->X + 0, Y + port->Y + 1, 0.3);
		grid->add_soft_cost(X + port->X + 0, Y + port->Y - 1, 0.3);
		grid->add_soft_cost(X + port->X - 1, Y + port->Y - 1, 0.6);
		grid->add_soft_cost(X + port->X - 1, Y + port->Y + 0, 0.3);
		grid->add_soft_cost(X + port->X - 1, Y + port->Y + 1, 0.6);
	}

	for (auto port : ports)
		port->fill_grid(grid);
}

void element::update_value(class port *port, struct value *value, int ttl)
{
	if (memcmp(value, &port->value, sizeof(struct value)) == 0)
		return; /* short circuit noop updates */

	notify(ttl -1);

}

void element::notify(int ttl)
{
	if (ttl <= 0)
		return;
	calculate(ttl -1);
}

void element::calculate(int ttl)
{
	/* empty in the base class */
}

bool element::mouse_select(float X, float Y)
{
	reseat();
	return false;
}

class wire *element::is_wire(float X, float Y)
{
	class wire *wire = NULL;
	for (auto port : ports) {
		wire = port->is_wire(X, Y);
		if (wire)
			return (wire);
	}
	return NULL;
}

void element::reseat(void)
{
	for (auto port: ports) {
		port->screenX = X + port->X;
		port->screenY = Y + port->Y;
	}

}

void element::to_json(json &j)
{
	unsigned int i;
	j["class_id"] = class_id();
	j["sizeX"] = sizeX;
	j["sizeY"] = sizeY;
	j["X"] = X;
	j["Y"] = Y;
	j["name"] = name;
	j["verilog_name"] = verilog_name;
	j["parental_name"] = parental_name;
	j["uuid"] = uuid;
	j["ports"] = json::array();
	j["angle"] = angle;
	for (i = 0; i < ports.size(); i++) {
		json p;
		ports[i]->to_json(p);
		j["ports"][i] = p;
	}
}

void element::from_json(json &j)
{
	unsigned int i;
	sizeX = j["sizeX"];
	sizeY = j["sizeY"];
	X = j.value("X", 0);
	Y = j.value("Y", 0);
	angle = j.value("angle", 0);
	uuid = j.value("uuid", generate_semi_uuid());
	name = j["name"];
	parental_name = j.value("parental_name", "");
	verilog_name = j.value("verilog_name", "");

	while (ports.size() < j["ports"].size())
		add_port(0,0, "");
	for (i = 0; i < j["ports"].size(); i++) {
		json jj = j["ports"][i];
		ports[i]->from_json(jj);
	}
	reseat();
	add_nets();
}

bool element::want_deleted(void)
{
	return want_delete;
}


void element::delete_if_selected(void)
{

	if (!selected)
		return;

	delete_element();
}
void element::delete_element(void)
{

	for (auto port:ports) {
		port->remove_wires();
		delete port;
	}	
	ports.clear();
	want_delete = true;
}

void element::remove_orphans(void)
{
	for (auto port:ports) {
		port->remove_orphans();
	}	
}

/* needed for cut-and-paste so that pasted elements have unique UUIDs */
void element::reset_uuid(void)
{
	uuid = generate_semi_uuid();
}


static float dist(float X1, float Y1, float X2, float Y2)
{
	return sqrtf( (X2-X1)*(X2-X1) + (Y2-Y1)*(Y2-Y1));
}


void element::hover_ports(class basecanvas *canvas)
{
	for (auto port:ports) {
		int dx = 0, dy = 0;
		if (port->X == -1) dx = 1;
		if (port->X == sizeX) dx = - 1;
		if (port->Y == -1) dy = 1;
		if (port->Y == sizeY) dy = -1;

		if (dist(mouseX, mouseY, X + port->X, Y + port->Y) < 10) {
			canvas->draw_image("assets/gray.png", X + port->X + dx, Y + port->Y +dy, 1, 1);
			canvas->draw_text(port->name, X + port->X + dx, Y + port->Y +dy, 1, 1);
		}
	}
}

void element::handle_event(class basecanvas *thiscanvas, SDL_Event &event)
{
	switch (event.type) {
		case SDL_MOUSEMOTION:
			mouseX = thiscanvas->scr_to_X(event.motion.x); 
			mouseY = thiscanvas->scr_to_Y(event.motion.y);

			break;
	}
}


void element::connect_clk(class port *clk)
{
	printf("COnnecting clock %s \n", name.c_str());
	for (auto port : ports) {
		if (port->name == "clk" && port->direction == PORT_IN && !port->has_wires()) {
			class wire *wire;
			wire = new class wire(0,0,0,0);
			wire->add_port(clk);
			wire->add_port(port);

			port->add_wire(wire);
			clk->add_wire(wire);

			wire->reseat();
		}
	}
}

void element::cycle_color(void)
{
	for (auto port : ports)
		port->cycle_color();
}

void element::reroute_all_wires(void)
{
	for (auto port : ports)
		port->update_distances();
	for (auto port : ports)
		port->reroute_all_wires();
}

void element::create_verilog_name(int seqno, std::vector<std::string> *existing)
{

	if (name != "") {
		verilog_name = name;
	} else { 
		verilog_name = class_id();
		verilog_name.pop_back();
	}
	std::replace(verilog_name.begin(), verilog_name.end(), '-', '_');
	std::replace(verilog_name.begin(), verilog_name.end(), ' ', '_');
	std::replace(verilog_name.begin(), verilog_name.end(), '+', '_');

	for (auto s : *existing) {
		if (s == verilog_name) {
			verilog_name = verilog_name + "_" + std::to_string(seqno);
		}
	}
	existing->push_back(verilog_name);
}


void element::collect_nets(std::vector<std::string> *netmap)
{
	for (auto port : ports)
		port->collect_nets(netmap);
}

void element::rotate_ports(void)
{
	for (auto port : ports) {

		if (port->X == -1) {
			port->X = sizeY -1 - port->Y;
			port->Y = -1;
		} else if (port->Y == -1) {
			port->Y = port->X;
			port->X = sizeY;
		} else if (port->X == sizeX) {
			port->X = sizeY - 1 - port->Y;
			port->Y = sizeX;
		}  else { /* port-Y == sizeY; */
			port->Y = port->X;
			port->X = -1;
		}


		port->screenX = X + port->X;
		port->screenY = Y + port->Y;

		port->route_wires();
	}
	float tmp = sizeY;
	sizeY = sizeX;
	sizeX = tmp;
	reseat();
	angle += 90;
	if (angle >= 360)
		angle -= 360;
	if (angle < 0)
		angle += 360;
}


void element::remove_nets(void)
{
	for (auto port : ports)
		port->remove_net();
}

void element::add_nets(void)
{
	for (auto port : ports) {
		if (port->has_net())
			continue;
		class net *net = new class net();
		net->add_port(port);
		net->validate();
		net->update_net_distances();
		my_nets.push_back(net);
	}
}

void element::update_value_net(struct value *value, int port, int ttl)
{
	add_nets();
	ports[port]->update_value_net(value, ttl);
}

void element::free_nets_memory(void)
{
	remove_nets();
	for (auto n : my_nets) {
		delete n;
	}
	my_nets.clear();
}

int element::has_port(class port *p)
{
	for (unsigned int i = 0; i < ports.size(); i++)
		if (ports[i] == p)
			return i;
	return -100;
}

void element::validate_ports(void)
{
	for (auto port : ports)
		port->validate();
}
