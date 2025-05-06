#include <assert.h>
#include <math.h>

#include "gridcad.h"

element::element(int _sizeX, int _sizeY, const char *_name)
{
    assert(_sizeX >= 2);
    assert(_sizeY >= 2);
    sizeX = _sizeX;
    sizeY = _sizeY;
    place(0,0);
    if (_name)
        name = strdup(_name);
    else
        name = strdup("NONAME"); /* TODO: generate a unimque name */
        
    
    add_port(0, 1, "In1");
    add_port(0, 3, "In1");
    add_port(1, 0, "clk");
    add_port(sizeX, 1, "Out1");
    add_port(sizeX, 3, "Out3");
}

element::~element()
{
    free((void*)name);
    for (auto port : ports) {
        free(port);
    }
}

void element::place(int _X, int _Y)
{
    X = _X; Xghost = _X; Xdnd = _X;
    Y = _Y; Yghost = _Y; Ydnd = _Y;
}

void element::drawConnector(class scene *scene, float X, float Y, int cX, int cY, int type)
{
    scene->drawCircle(cX + X + 0.5, cY+ Y + 0.5, 0.48, type);
}

void element::drawAt(class scene *scene, float X, float Y, int type)
{
    assert (type <= DRAW_ORIGIN);
    
    scene->drawBox(X + 1, Y + 1, X + sizeX , Y + sizeY , COLOR_ELEMENT_NORMAL + type);
    if (type == DRAW_NORMAL || type == DRAW_ORIGIN)
        scene->drawBox(X + 1.2, Y + 1.2, X + sizeX - 0.2, Y + sizeY - 0.2, COLOR_ELEMENT_INSIDE);
    
    /* TODO: real connectors */
    for (auto port: ports) {
        drawConnector(scene, X, Y, port->X, port->Y, COLOR_ELEMENT_CONNECTOR + type);    
    }
    for (auto wire : wires) {
        wire->draw(scene);
    }
}


void element::draw(class scene *scene, int type)
{
    switch (type) {
        case DRAW_NORMAL:
            drawAt(scene, X, Y, DRAW_NORMAL);
            break;
        case DRAW_GHOST:
            drawAt(scene, Xghost, Yghost, DRAW_GHOST);
            break;
        case DRAW_DND:
            drawAt(scene, Xdnd, Ydnd, DRAW_DND);
            break;
        case DRAW_ORIGIN:
            drawAt(scene, X, Y, DRAW_ORIGIN);
            break;
            
        default:
            assert(0);
        
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

void element::update_drag(class scene *scene, float _X, float _Y)
{
    if (fabsf(_X - X) > 2 || fabsf(_Y - Y) > 2)
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

void element::stop_drag(class scene *scene)
{
    X = Xghost;
    Y = Yghost;
    over_drag_threshold = false;
    for (auto wire : wires) {
        wire->reseat();
        wire->route(scene);
    }
}


/* does a mouse press hit this specific element */
bool element::intersect(float _X, float _Y)
{
    if (_X >= X + 1 && _Y >= Y + 1 && _X < X + sizeX && _Y < Y + sizeY)
        return true;
    return false;
}

void element::add_port(int X, int Y, const char *_name)
{
    struct port *port;
    port = (struct port*)calloc(sizeof(struct port), 1);
    port->X = X;
    port->Y = Y;
    port->name = strdup(_name);
    port->parent = this;
    ports.push_back(port);
}

struct port * element::is_port(float _X, float _Y)
{
    /* make relative to the element */
    _X -= X;
    _Y -= Y;
    
    for (auto port : ports) {
        float d = sqrtf(  (_X - port->X - 0.5) * (_X - port->X - 0.5) + (_Y - port->Y - 0.5) * (_Y - port->Y - 0.5));
        if (d > 0.5) 
            continue;
        return port;
    }
    return NULL;
}

void element::fill_grid(class wiregrid *grid)
{
    int x,y;

    for (x = 1; x < sizeX; x++)
        for (y = 1; y < sizeY; y++)
            grid->block_point(X+x, Y+y);

    for (x = 0; x < sizeX + 1; x++)
        for (y = 0; y < sizeY +1; y++)
            grid->add_soft_cost(X+x, Y+y, 0.6);

    for (auto port: ports) {
        grid->block_point(X + port->X, Y + port->Y);
        grid->add_soft_cost(X + port->X + 1, Y + port->Y + 0, 0.3);
        grid->add_soft_cost(X + port->X - 1, Y + port->Y + 0, 0.3);
        grid->add_soft_cost(X + port->X + 0, Y + port->Y + 1, 0.3);
        grid->add_soft_cost(X + port->X + 0, Y + port->Y - 1, 0.3);
        grid->add_soft_cost(X + port->X + 1, Y + port->Y + 1, 0.6);
        grid->add_soft_cost(X + port->X - 1, Y + port->Y - 1, 0.6);
        grid->add_soft_cost(X + port->X - 1, Y + port->Y + 1, 0.6);
        grid->add_soft_cost(X + port->X - 1, Y + port->Y - 1, 0.6);
    }
}

void element::add_wire(class wire *wire, struct port *port)
{
    wire->get_ref();
    wires.push_back(wire);
    wire->add_parent(port);
}