#include "gridcad.h"

connector::connector(float _X, float _Y)  : element(3, 3, NULL)
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
    int color = COLOR_VALUE_RED;
    
    if (ports[0]->value.boolval)
        color = COLOR_VALUE_GREEN;
    for (auto port: ports) 
        port->draw_wires(canvas);
    canvas->drawCircle(X+0.5, Y+0.5, 0.5, COLOR_WIRE_SOLID, color);
}

void connector::fill_grid(class wiregrid *grid)
{
    grid->block_point(X, Y);
}

