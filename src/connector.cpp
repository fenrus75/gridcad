#include "gridcad.h"

connector::connector(float _X, float _Y)  : element(3, 3, "")
{
    sizeX = 1;
    sizeY = 1;    
    X = floorf(_X);
    Y = floorf(_Y);

    add_port(0, 0, "Connector", PORT_INOUT);    
    reseat();;
}

connector::~connector(void)
{
}

void connector::draw(class canvas *canvas, int type)
{
    if (red == NULL)
        red = canvas->load_image("assets/connector_red.png");
    if (green == NULL)
        green = canvas->load_image("assets/connector_green.png");
    if (open == NULL)
        open = canvas->load_image("assets/port_in_open.png");
    
    for (auto port: ports) 
        port->draw_wires(canvas);
        
    if (ports[0]->value.boolval)
        canvas->draw_image(green, X,Y,1,1);
    else
        canvas->draw_image(red, X,Y,1,1);
}

void connector::fill_grid(class wiregrid *grid)
{
    grid->block_point(X, Y);
    grid->add_soft_cost(X + 1, Y + 1, 0.3);
    grid->add_soft_cost(X + 1, Y - 1, 0.3);
    grid->add_soft_cost(X - 1, Y + 1, 0.3);
    grid->add_soft_cost(X - 1, Y - 1, 0.3);
}


bool connector::want_deleted(void)
{
    if (!ports[0]->has_wires())
        return true;
    return false;
}