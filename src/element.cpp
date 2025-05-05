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
    name = strdup(_name);
    
    add_port(0, 1, "In1");
    add_port(0, 2, "In2");
    add_port(0, 3, "Enable");
    add_port(1, 0, "clk");
    add_port(2, 0, "reset");
    add_port(sizeX, 1, "Out1");
    add_port(sizeX, 2, "Out2");
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
    scene->drawCircle(cX + X, cY+ Y, 0.24, type);
}

void element::drawAt(class scene *scene, float X, float Y, int type)
{
    assert (type <= DRAW_ORIGIN);
    
    scene->drawBox(X + 0.25, Y + 0.25, X + sizeX - 0.25, Y + sizeY - 0.25, COLOR_ELEMENT_NORMAL + type);
    if (type == DRAW_NORMAL || type == DRAW_ORIGIN)
        scene->drawBox(X + 0.45, Y + 0.45, X + sizeX - 0.45, Y + sizeY - 0.45, COLOR_ELEMENT_INSIDE);
    
    /* TODO: real connectors */
    for (auto port: ports) {
        drawConnector(scene, X, Y, port->X, port->Y, COLOR_ELEMENT_CONNECTOR + type);    
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

void element::stop_drag(void)
{
    X = Xghost;
    Y = Yghost;
    over_drag_threshold = false;
}


/* does a mouse press hit this specific element */
bool element::intersect(float _X, float _Y)
{
    if (_X > X + 0.25 && _Y > Y + 0.25 && _X < X + sizeX - 0.25 && _Y < Y + sizeY - 0.25)
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
        float d = sqrtf(  (_X - port->X) * (_X - port->X) + (_Y - port->Y) * (_Y - port->Y));
        if (d > 0.27) 
            continue;
        return port;
    }
    return NULL;
}