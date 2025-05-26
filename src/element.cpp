#include <assert.h>
#include <math.h>

#include "gridcad.h"
#include "port.h"

element::element(int _sizeX, int _sizeY, std::string _name)
{
    sizeX = _sizeX;
    sizeY = _sizeY;
    place(0,0);
    name = _name;
    uuid = generate_semi_uuid();
}

element::~element()
{
    for (auto port : ports) {
        delete port;
    }
}

void element::place(int _X, int _Y)
{
    X = _X; Xghost = _X; Xdnd = _X;
    Y = _Y; Yghost = _Y; Ydnd = _Y;
}


void element::drawAt(class canvas *canvas, float X, float Y, int type)
{
    assert (type <= DRAW_ORIGIN);
    
    canvas->draw_box(X + 1, Y + 1, X + sizeX , Y + sizeY , COLOR_ELEMENT_NORMAL + type);
    if (type == DRAW_NORMAL || type == DRAW_ORIGIN)
        canvas->draw_box(X + 1.2, Y + 1.2, X + sizeX - 0.2, Y + sizeY - 0.2, COLOR_ELEMENT_INSIDE);
    
    for (auto port: ports) {
        port->drawAt(canvas, X, Y, type);
    }
}


void element::draw(class canvas *canvas, int type)
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

void element::draw_phase2(class canvas *canvas, int type)
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

void element::update_drag(class canvas *canvas, class scene *scene, float _X, float _Y)
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

void element::stop_drag(class canvas *canvas)
{
    X = Xghost;
    Y = Yghost;
    
    reseat();
    over_drag_threshold = false;
    
    for (auto port : ports)
        port->stop_drag(canvas);

    calculate(DEFAULT_TTL);
}


/* does a mouse press hit this specific element */
bool element::intersect(float _X, float _Y)
{
    if (_X >= X && _Y >= Y  && _X < X + sizeX && _Y < Y + sizeY)
        return true;
    return false;
}

void element::add_port(int X, int Y, const char *_name, int direction, bool initval)
{
    class port *_port;
    _port = new port(_name, direction);
    _port->X = X;
    _port->Y = Y;
    _port->parent = this;
    _port->value.boolval = initval;
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
    j["uuid"] = uuid;
    j["ports"] = json::array();
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
    X = j["X"];
    Y = j["Y"];
    uuid = j.value("uuid", generate_semi_uuid());
    name = j["name"];
    
    while (ports.size() < j["ports"].size())
        add_port(0,0, "");
    for (i = 0; i < j["ports"].size(); i++) {
        json jj = j["ports"][i];
        ports[i]->from_json(jj);
    }
    reseat();
}

bool element::want_deleted(void)
{
    return want_delete;
}


void element::delete_if_selected(void)
{

	if (!selected)
		return;

	for (auto port:ports) {
		port->remove_wires();
	}	
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