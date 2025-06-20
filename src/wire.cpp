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
#include "wirepath.h"
#include "wire.h"
#include "port.h"
#include "net.h"
#include "model_clock.h"

#include <algorithm>

#include <sys/time.h>
#include <map>

extern bool wire_debug_mode ;

wire::wire(int x1, int y1, int x2, int y2, int _color)
{
    X1 = x1;
    Y1 = y1;
    X2 = x2;
    Y2 = y2;
    color = _color;
    refcount = 1;
    
    points = NULL;
    value = {};
    name = generate_wire_name();;
    
}

wire::~wire(void)
{
    if (points)
        delete points;
}

void wire::set_new_name(void)
{
    name = generate_wire_name();

}

/* returns a value of 0 - 60 based on the current time, suitable for animations */
/* TODO: this should be a global value for a whole frame -- not for each individual wire */
static int current_interval(void)
{
    struct timeval tv;
    double d;
    gettimeofday(&tv, NULL);
    d = tv.tv_usec;
    
    d = d * 60 / 1000000.0;
    return floorf(d);        
}

float dist(float x1, float y1, float x2, float y2)
{
        float dx, dy,d;
        dx = x2-x1;
        dy = y2-y1;
        d = sqrtf(dx*dx + dy*dy);
        return d;
}

float distsq(float x1, float y1, float x2, float y2)
{
        float dx, dy,d;
        dx = x2-x1;
        dy = y2-y1;
        d = dx*dx + dy*dy;
        return d;
}

int calc_angle(int x1, int y1, int x2, int y2)
{
    int dx, dy;
    
    dx = x2 - x1;
    if (dx > 1) dx = 1;
    if (dx < -1) dx = -1;
    dy = y2 - y1;
    if (dy > 1) dy = 1;
    if (dy < -1) dy = -1;
    
    
    if (dx ==  1 && dy ==  0) return 180;
    if (dx ==  1 && dy ==  1) return 45;
    if (dx ==  0 && dy == -1) return 270;
    if (dx ==  1 && dy == -1) return 135;
    if (dx == -1 && dy ==  0) return 0;
    if (dx == -1 && dy == -1) return 225;
    if (dx ==  0 && dy ==  1) return 90;
    if (dx == -1 && dy ==  1) return 315;
    
    
    return 0;
}

void draw_snake_line(class canvas *canvas, float x1, float y1, float x2, float y2, int color, int *step, struct value *value, int stepsize, class wire *wire)
{
        double dx, dy,d;
        bool thick = false;
        int ang = calc_angle(roundf(x1),roundf(y1),roundf(x2),roundf(y2));
	int totalstep = 0;
        
        if (value->type == VALUE_TYPE_INT)
            thick = true;

        if (thick)         
            canvas->draw_thick_line(x1, y1, x2, y2, color);
        else
            canvas->draw_line(x1, y1, x2, y2, color);

        if (!canvas->canvas_has_focus())
           return;
        
        dx = x2-x1;
        dy = y2-y1;
        d = dist(x1,y1,x2,y2);;
        dx = dx/d/12.0;
        dy = dy/d/12.0;
        
        
        while (distsq(x1,y1,x2,y2) > (1/10.0)*(1/10.0)) {
            
            
            (*step)++;
	    totalstep++;
            if ((*step) >= stepsize) {
                (*step) = 0;
		if (value->type == VALUE_TYPE_INT || wire_debug_mode || wire->get_is_z()) { 
                       float cursormag;
                        cursormag = 2.5 - canvas->distance_from_mouse(x1,y1)/2;
                        if (cursormag < 1)
                            cursormag = 1;
                        if (cursormag > 2)
                	    cursormag = 2;
			const float size = 0.24 * cursormag;
			const float size2 = 0.20 * cursormag;
			char buf[128];
			std::string s;
			sprintf(buf, "%li", value->intval);
			
			if (wire->get_is_z())
			    sprintf(buf, "%c", 'Z');
			if (wire_debug_mode)
			    sprintf(buf, "%u", wire->get_distance_from_outport());
			s = buf;
			canvas->draw_box(x1-size, y1-size, x1+size, y1+size, COLOR_WIRE_MOTION);
			canvas->draw_box(x1-size2, y1-size2, x1+size2, y1+size2, COLOR_BACKGROUND_MAIN);
	                canvas->draw_text(s, x1+0.05-size, y1+0.05-size, 2 * size - 0.1, 2 * size - 0.1);
		} else if (value->is_clock) {
    			const float size = 0.24;
                        float f;
			f = 2 * totalstep / (0.01 + stepsize);
                        while (f > 1)
                            f -= 1;

			f = 1 -f;
                        f = f * (450-25);
                        
                        if (!clock_running)
                            f = 0;
			canvas->draw_box(x1-size, y1-size, x1+size, y1+size, COLOR_WIRE_MOTION);
//                        canvas->draw_image_fragment("assets/clock_wave.png", x1-size, y1-size, 4*size, 2*size, 25 + f, 0, 2 * (450 -25), 189, ang);	
                        canvas->draw_image_fragment("assets/clock_wave.png", x1-size, y1-size, 2*size, 2*size, 25 + f, 0, 450 -25, 189, ang);	
		} else {
		        float r = 0.18;
		        if (thick)
		            r = 0.22;
	                canvas->draw_circle2(x1, y1, r, COLOR_WIRE_MOTION, value_color(value));
		}
            }
            x1 += dx;
            y1 += dy;
        }
        
        
    
}

void wire::draw(class canvas *canvas, int _color)
{
    int prevX, prevY;
    bool first = true;
    int stepsize;
    int step;

    assert(!dead);
    
    if (!points)
        route(canvas->get_scene());

    if (!points)
        return;
        
    stepsize = 60;
    if (selected)
        stepsize = 20;
        
    step = current_interval() % stepsize;
    
    if (value.is_clock && !clock_running)
        step = (global_clock.arrayval * 4) % stepsize;;
    for (auto point: *points) {
        if (first) {
            prevX = point.X;
            prevY = point.Y;
            first = false;
            continue;
        }
        canvas->draw_shadow_Line(prevX + 0.5, prevY + 0.5, point.X + 0.5, point.Y + 0.5, color);
        prevX = point.X;
        prevY = point.Y;
    }
    first =true;
    
    /* only draw the snake line on windows that have the focus -- it's resource intensive */
    for (auto point: *points) {
        if (first) {
            prevX = point.X;
            prevY = point.Y;
            first = false;
            continue;
        }
        draw_snake_line(canvas, prevX + 0.5, prevY + 0.5, point.X + 0.5, point.Y + 0.5, wire_to_color(color), &step, &value, stepsize, this);
        prevX = point.X;
        prevY = point.Y;
    }
}

void wire::move_target(int X, int Y)
{
    if (X2 == X && Y2 == Y)
        return;
    X2 = X;
    Y2 = Y;
    if (points)
        delete points;
    points = NULL;
}

void wire::route(class scene *scene)
{
    being_routed = true;
    if (points)
        delete points;
        
    printf("Routing wire\n");
        
    class wiregrid * grid = new wiregrid(scene->sizeX, scene->sizeY);

    grid->is_clock = value.is_clock;
    
    scene->fill_grid(grid);
    
    points = grid->path_walk(X1, Y1, X2, Y2, value.is_clock);
//    printf("Walk is %i %i -> %i %i \n", X1, Y1, X2, Y2);
//    for (auto p: *points) {
//        printf("X, Y : %i, %i\n", p.X, p.Y);
//    }
//    printf("----\n");
    delete(grid);

    bool want_reverse = false;
	
    for (auto port : ports) {
        if (floorf(port->screenX) == X1 && floorf(port->screenY) == Y1 && port->direction == PORT_OUT) {
	    want_reverse = true;
        }
        if (floorf(port->screenX) == X2 && floorf(port->screenY) == Y2 && port->direction == PORT_IN) {
	    want_reverse = true;
        }
    }
    
    if (ports.size() > 1) {
        if (floorf(ports[0]->screenX) == X1 && floorf(ports[0]->screenY) == Y1 && ports[0]->get_distance_from_outport() < ports[1]->get_distance_from_outport()) 
            want_reverse = true;
    }
    if (want_reverse) 
            std::reverse(points->begin(), points->end());
    is_reversed = want_reverse;
    being_routed = false;
    reseat();
}

void wire::check_reverse(void)
{
    being_routed = true;
    bool want_reverse = false;
    reseat();
    
    if (!points)
        return;

    for (auto port : ports) {
        if (floorf(port->screenX) == X1 && floorf(port->screenY) == Y1 && port->direction == PORT_OUT) {
	    want_reverse = true;
        }
        if (floorf(port->screenX) == X2 && floorf(port->screenY) == Y2 && port->direction == PORT_IN) {
	    want_reverse = true;
        }
    }
    
    if (ports.size() > 1) {
        if (floorf(ports[0]->screenX) == X1 && floorf(ports[0]->screenY) == Y1 && ports[0]->get_distance_from_outport() < ports[1]->get_distance_from_outport()) 
            want_reverse = true;
    }
    if (want_reverse != is_reversed && points->size()>1)
            std::reverse(points->begin(), points->end());
    being_routed = false;
    is_reversed = want_reverse;
    reseat();
}

void wire::get_ref(void)
{
    refcount++;
}

void wire::add_port(class port *port)
{
    for (auto _port : ports) {
        if (port == _port)
            return;
    }
    ports.push_back(port);
    if (port->direction != PORT_IN) {
        update_value_net(&port->value, DEFAULT_TTL);
    }
}

void wire::del_port(class port *port)
{
    for (unsigned int i = 0; i< ports.size(); i++) {
        auto _port = ports[i];
        if (port == _port) {
            ports.erase(ports.begin() + i);
            return;
        }
    }
}
void wire::reseat(void)
{
    if (ports.size() > 0) {
        X1 = ports[0]->screenX;
        Y1 = ports[0]->screenY;
    }
    if (ports.size() > 1) {
        X2 = ports[1]->screenX;
        Y2 = ports[1]->screenY;

        if (ports[0]->value_ttl > ports[1]->value_ttl && ports[0]->value_ttl && ports[1]->value_ttl) {
            printf("Flip wires\n");
            int tmpX, tmpY;
            tmpX = X1; tmpY = Y1;
            X1 = X2;
            Y1 = Y2;
            X2 = tmpX;
            Y2 = tmpY;
        }
    }
}
#if 0
void wire::update_value(struct value *newvalue, int ttl)
{
    if (ttl <= 0)
        return;
    if (!newvalue->valid)
	return;
    if (value.valid && memcmp(&value, newvalue, sizeof(struct value)) == 0) {
        /* no change -- early exit to kill oscillations */
        return;
    }
    value = *newvalue;
    if (ttl < 10) {
        printf("wire ttl exceeded %i \n", ttl);
        newvalue->is_error = true;
    }
    
    /* now to notify the ports we're connected to */
    for (auto port: ports) {
        if (port->direction != PORT_OUT)
            port->update_value(&value, ttl -1);
    }
    notify(ttl - 1);
}
#endif

void wire::update_value_final(struct value *newvalue, int ttl)
{
    value = *newvalue;
    notify(ttl - 1);
}

void wire::update_value_net(struct value *newvalue, int ttl)
{
    if (ttl <= 0)
        return;

    if (!net)
	return;

    net->update_value(newvalue, ttl - 1);
}

void wire::notify(int ttl)
{
}


bool wire::intersect(float targetX, float targetY)
{
    float bestdist = 10000;
    bool first = true;
    float prevX, prevY;

    if (!points)
	return false;
    
    /* TODO we should do fancy "point to a line" math.... */
    for (auto point: *points) {
        if (first) {
            first = false;
            prevX = point.X + 0.5;
            prevY = point.Y + 0.5;
            continue;
        }
        double dx, dy,d;
        float x1,y1,x2,y2;
        x1 = prevX;
        y1 = prevY;
        x2 = point.X + 0.5;
        y2 = point.Y + 0.5;

#if 0
	/* bounding box first */
	if (targetX < x1 - 2 && targetX < x2 - 2)
		continue;
	if (targetX > x2  + 2 && targetX > x1 + 2)
		continue;
	if (targetY < y1 - 2 && targetY < y2 - 2)
		continue;
	if (targetY > y2  + 2 && targetY > y1 + 2)
		continue;
#endif
    
        dx = x2-x1;
        dy = y2-y1;
        d = dist(x1,y1,x2,y2);
        dx = dx/d/20.0;
        dy = dy/d/20.0;
        
        
        while (dist(x1,y1,x2,y2) > 1/10.0) {
            if (dist(targetX, targetY, x1, y1) < bestdist) {
                bestdist = dist(targetX, targetY, x1, y1);
            }
            x1 += dx;
            y1 += dy;            
        }
        prevX = point.X + 0.5;
        prevY = point.Y + 0.5;
    }

    if (bestdist <= 0.5)
        return true;
    return false;
}

void wire::remove(void)
{
    for (auto port : ports) {
        port->remove_wire(this);
    }
    if (points)
        delete points;
    points = NULL;
}


/* returns a new wire. After this call both
the current and new wire have one loose side */
class wire *wire::split(void)
{
    class wire *wr = new wire(0, 0, 0, 0, color);

    printf("Splitting wire\n");
    wr->update_value_final(&value, 100);    
    wr->set_width(get_width());
    /* upper bound the distance from outports */
    wr->set_distance_from_outport(distance_from_outport + 1);
    distance_from_outport++;
    if (ports.size() > 0) {
        ports[0]->replace_wire(this, wr);
        ports.erase(ports.begin() + 0);
    }
    wr->reseat();
    wr->push_wire_color(color);
    reseat();
    
    return wr;
}

void wire::to_json(json &j)
{
    j["value"] = value;
    j["name"] = name;
    j["X1"] = X1;
    j["Y1"] = Y1;
    j["X2"] = X2;
    j["Y2"] = Y2;
    j["color"] = color;
    j["width"] = width;
    j["distance_from_outport"] = distance_from_outport;
    j["is_reversed"] = is_reversed;
    if (points)
        j["points"] = *points;
}    
void wire::from_json(json &j)
{
    value.is_clock = false;
    value = j["value"];
    name = j["name"];
    X1 = j["X1"];
    Y1 = j["Y1"];
    X2 = j["X2"];
    Y2 = j["Y2"];
    color = j["color"];
    width = j.value("width", 0);
    is_reversed = j.value("is_reversed", false);
    distance_from_outport = j.value("distance_from_outport", INT_MAX);
    if (j.contains("points")) {
        points =  new std::vector<struct waypoint>;
        *points = j["points"];
    }
}


static std::map<std::string, class wire*> json_wires;

void clear_wire_factory(void)
{
	json_wires.clear();
}

bool wire_factory_force_new_name;

class wire *json_wire_factory(json &jwire)
{
    class wire *_wire;
    if (json_wires.find(jwire["name"]) != json_wires.end()) {
        std::string s = jwire["name"];
        _wire = json_wires[s];
        json_wires.erase(s);
//        printf("Wire factory match found for %s  factory size %lu\n", s.c_str(), json_wires.size());
        return _wire;
    }
    class wire *new_wire = new wire(jwire["X1"], jwire["Y1"], jwire["X2"], jwire["Y2"]);
    
    json_wires[jwire["name"]] = new_wire;
    
    new_wire->from_json(jwire);
    
//    printf("New wire created: %s  total factory size %lu\n", new_wire->name.c_str(), json_wires.size());
    
    if (wire_factory_force_new_name)
        new_wire->set_new_name();
    
    return new_wire;
}



static void grid_line(class wiregrid *grid, float x1, float y1, float x2, float y2, bool is_clock)
{
        double dx, dy,d;
	float self_cost = 2.25;
	if (is_clock && grid->is_clock)
		self_cost = -0.05; /* we don't mind clocks routing over clocks */
        
        dx = x2-x1;
        dy = y2-y1;
        d = dist(x1,y1,x2,y2);;
        dx = dx/d/12.0;
        dy = dy/d/12.0;
        
        
        while (dist(x1,y1,x2,y2) > 1/10.0) {
	    if (!is_clock)
		    grid->declare_part_of_wire(x1, y1);
            if (grid->get_soft_cost(x1, y1) < 0.001)
                grid->add_soft_cost(x1, y1, self_cost);
            if (grid->get_soft_cost(x1 - 1, y1) < 0.001)
                grid->add_soft_cost(x1 - 1, y1, -0.05);
            if (grid->get_soft_cost(x1 + 1, y1) < 0.001)
                grid->add_soft_cost(x1 + 1, y1, -0.05);
            if (grid->get_soft_cost(x1, y1 - 1) < 0.001)
                grid->add_soft_cost(x1, y1 - 1, -0.05);
            if (grid->get_soft_cost(x1, y1 + 1) < 0.001)
                grid->add_soft_cost(x1, y1 + 1, -0.05);
            x1 += dx;
            y1 += dy;
        }
        
        
    
}

void wire::fill_grid(class wiregrid *grid)
{
    int prevX, prevY;
    bool first = true;
        
    if (!points)
        return;
    if (being_routed)
        return;
        
    first =true;
    for (auto point: *points) {
        if (first) {
            prevX = point.X;
            prevY = point.Y;
            first = false;
            continue;
        }
        grid_line(grid, prevX + 0.5, prevY + 0.5, point.X + 0.5, point.Y + 0.5, value.is_clock);
        prevX = point.X;
        prevY = point.Y;
    }
}

void wire::remove_if_orphan(void)
{
	if (ports.size() < 2) {
	        printf("Removing orphan  %lu\n",ports.size());
		remove();
        }
}

class port *wire::get_other_port(class port *port)
{
    for (auto p : ports) {
        if (p != port)
            return p;
    }
    return NULL;
}

void wire::push_wire_color(int _color)
{
	if (color == _color)
		return;
	color = _color;
return;
	for (auto port : ports)
		port->push_wire_color(color);
}

std::string wire::get_verilog_decl(void)
{
    std::string s;
    
    s = "wire ";
    if (width > 1)
    s = s + "[" + std::to_string(width -1) + ":0] ";
    
    s = s  + get_verilog_name();
    s = s + ";";
    
    return s;
}

std::string wire::get_verilog_name(void)
{
    std::string s = name;
    std::replace(s.begin(), s.end(), '-', '_');
    s = s + std::to_string(distance_from_outport);
    return s;
    
}

void wire::add_net(class net *newnet)
{
	if (net == newnet)
		return;

	net = newnet;

	for (auto port : ports)
		port->add_net(newnet);

	net->add_wire(this);
}

void wire::remove_net(void)
{
	class net *save;
	if (!net)
		return;
	save = net;
	net = NULL;
	for (auto port : ports) {
		port->remove_net();	 
		save->remove_port(port);
	}

	save->remove_wire(this);
}

bool wire::update_distances(void)
{	
	unsigned int best = INT_MAX;

	for (auto port:ports)
		if (port->get_distance_from_outport() < best)
			best = port->get_distance_from_outport();

	if (distance_from_outport > best + 1) {
		distance_from_outport = best + 1;
		return true;
	}
	return false;
}