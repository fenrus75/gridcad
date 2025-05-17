
#include "gridcad.h"

#include <algorithm>

#include <sys/time.h>

static int wirecount = 0;

wire::wire(int x1, int y1, int x2, int y2, int _color)
{
    char buffer[128];
    X1 = x1;
    Y1 = y1;
    X2 = x2;
    Y2 = y2;
    color = _color;
    refcount = 1;
    
    points = NULL;
    value = {};
    sprintf(buffer,"Wire%i", wirecount++);
    name = strdup(buffer);
    
}

wire::~wire(void)
{
    if (points)
        delete points;
}

/* returns a value of 0 - 60 based on the current time, suitable for animations */
static int current_interval(void)
{
    struct timeval tv;
    double d;
    gettimeofday(&tv, NULL);
    d = tv.tv_usec;
    
    d = d * 60 / 1000000.0;
    return floorf(d);        
}

double dist(float x1, float y1, float x2, float y2)
{
        double dx, dy,d;
        dx = x2-x1;
        dy = y2-y1;
        d = sqrt(dx*dx + dy*dy);
        return d;
}


void draw_snake_line(class canvas *canvas, float x1, float y1, float x2, float y2, int color, int *step, struct value *value, int stepsize = 60)
{
        double dx, dy,d;
        canvas->drawLine(x1, y1, x2, y2, color);
        
        dx = x2-x1;
        dy = y2-y1;
        d = dist(x1,y1,x2,y2);;
        dx = dx/d/12.0;
        dy = dy/d/12.0;
        
        
        while (dist(x1,y1,x2,y2) > 1/10.0) {
            (*step)++;
            if ((*step) >= stepsize) {
                (*step) = 0;
                canvas->drawCircle(x1, y1, 0.18, COLOR_WIRE_MOTION, value_color(value));
            }
            x1 += dx;
            y1 += dy;
        }
        
        
    
}

void wire::draw(class canvas *canvas, int color)
{
    int prevX, prevY;
    bool first = true;
    int stepsize;
    int step;
    if (!points)
        route(canvas->get_scene());
        
    if (!points)
        return;
        
    stepsize = 60;
    if (selected)
        stepsize = 20;
        
    step = current_interval() % stepsize;
    for (auto point: *points) {
        if (first) {
            prevX = point.X;
            prevY = point.Y;
            first = false;
            continue;
        }
        draw_snake_line(canvas, prevX + 0.5, prevY + 0.5, point.X + 0.5, point.Y + 0.5, color, &step, &value, stepsize);
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
    if (points)
        delete points;
        
    class wiregrid * grid = new wiregrid(scene->sizeX, scene->sizeY);
    
    scene->fill_grid(grid);
    
    points = grid->path_walk(X1, Y1, X2, Y2);
    printf("Walk is %i %i -> %i %i \n", X1, Y1, X2, Y2);
    for (auto p: *points) {
        printf("X, Y : %i, %i\n", p.X, p.Y);
    }
    printf("----\n");
    delete(grid);

    for (auto port : ports) {
        if (floorf(port->screenX) == X1 && floorf(port->screenY) == Y1 && port->direction != PORT_IN) {
            std::reverse(points->begin(), points->end());
        }
    }
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
        update_value(&port->value, DEFAULT_TTL);
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
    }
}

void wire::update_value(struct value *newvalue, int ttl)
{
    if (ttl <= 0)
        return;
    if (memcmp(&value, newvalue, sizeof(struct value)) == 0) {
        /* no change -- early exit to kill oscillations */
        return;
    }
    value = *newvalue;
    if (ttl < 50) {
        newvalue->is_error = true;
    }
    
    /* now to notify the ports we're connected to */
    for (auto port: ports) {
        if (port->direction != PORT_OUT)
            port->update_value(&value, ttl -1);
    }
    notify(ttl - 1);
}

void wire::notify(int ttl)
{
}


bool wire::intersect(float targetX, float targetY)
{
    float bestdist = 10000;
    bool first = true;
    float prevX, prevY;
    
    /* TODO we should do fancy "point to a line" math.... */
    for (auto point: *points) {
        if (first) {
            first = false;
            prevX = point.X;
            prevY = point.Y;
            continue;
        }
        double dx, dy,d;
        float x1,y1,x2,y2;
        x1 = prevX;
        y1 = prevY;
        x2 = point.X;
        y2 = point.Y;
        
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
        prevX = point.X;
        prevY = point.Y;
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
}


/* returns a new wire. After this call both
the current and new wire have one loose side */
class wire *wire::split(void)
{
    class wire *wr = new wire(0, 0, 0, 0, color);
    
    if (ports.size() > 0) {
        ports[0]->replace_wire(this, wr);
        ports.erase(ports.begin() + 0);
    }
         
    wr->reseat();
    reseat();
    
    return wr;
}