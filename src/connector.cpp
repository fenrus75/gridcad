#include "gridcad.h"

connector::connector(float _X, float _Y)  : element(3, 3, NULL)
{
    sizeX = 1;
    sizeY = 1;    
    X = floorf(_X);
    Y = floorf(_Y);

    add_port(0, 0, "Connector", PORT_INOUT);    
}

connector::~connector(void)
{
}

void connector::draw(class scene *scene, int type)
{
    scene->drawCircle(X+0.5, Y+0.5, 0.5, COLOR_WIRE_SOLID);
    for (auto port: ports) 
        port->draw_wires(scene);
}

void connector::fill_grid(class wiregrid *grid)
{
    grid->block_point(X, Y);
}

