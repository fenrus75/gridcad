
#include "gridcad.h"

#include <sys/time.h>

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


void draw_snake_line(class scene *scene, float x1, float y1, float x2, float y2, int color, int *step, struct value *value)
{
        double dx, dy,d;
        scene->drawLine(x1, y1, x2, y2, color);
        
        dx = x2-x1;
        dy = y2-y1;
        d = dist(x1,y1,x2,y2);;
        dx = dx/d/12.0;
        dy = dy/d/12.0;
        
        
        while (dist(x1,y1,x2,y2) > 1/10.0) {
            (*step)++;
            if ((*step) >= 60) {
                (*step) = 0;
                scene->drawCircle(x1, y1, 0.18, COLOR_WIRE_MOTION, value_color(value));
            }
            x1 += dx;
            y1 += dy;
        }
        
        
    
}

void wire::draw(class scene *scene, int color)
{
    int prevX, prevY;
    bool first = true;
    int step;
    if (!points)
        route(scene);
        
    if (!points)
        return;
        
    step = current_interval();
    for (auto point: *points) {
        if (first) {
            prevX = point.X;
            prevY = point.Y;
            first = false;
            continue;
        }
        draw_snake_line(scene, prevX + 0.5, prevY + 0.5, point.X + 0.5, point.Y + 0.5, color, &step, &value);
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
}

void wire::get_ref(void)
{
    refcount++;
}

void wire::add_parent(class port *port)
{
    parents.push_back(port);
    if (port->direction == PORT_OUT) {
        update_value(&port->value);
    }
}

void wire::reseat(void)
{
    if (parents.size() > 0) {
        X1 = parents[0]->parent->get_X() + parents[0]->X;
        Y1 = parents[0]->parent->get_Y() + parents[0]->Y;
    }
    if (parents.size() > 1) {
        X2 = parents[1]->parent->get_X() + parents[1]->X;
        Y2 = parents[1]->parent->get_Y() + parents[1]->Y;
    }
}

void wire::update_value(struct value *newvalue)
{
    if (memcmp(&value, newvalue, sizeof(struct value)) == 0) {
        /* no change -- early exit to kill oscillations */
        return;
    }
    value = *newvalue;
    
    /* now to notify the ports we're connected to */
    for (auto port: parents) {
        if (port->direction == PORT_IN)
            port->update_value(&value);
    }
}