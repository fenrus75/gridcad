#include "gridcad.h"
#include "models.h"

model_zero::model_zero(float _X, float _Y)  : element(1, 1, NULL)
{
    sizeX = 2;
    sizeY = 1;    
    X = floorf(_X);
    Y = floorf(_Y);
    
    ports.resize(0);

    add_port(1, 0, "ZERO", PORT_OUT);    
}

model_zero::~model_zero(void)
{
}

void model_zero::drawAt(class scene *scene, float X, float Y, int type)
{
    scene->drawBox(X,Y,X+1,Y+1, COLOR_ELEMENT_NORMAL);
    scene->drawCircle(X+1.5, Y+0.5, 0.5, COLOR_WIRE_SOLID);
    for (auto port : ports) {
        port->draw_wires(scene);
    }
}

void model_zero::fill_grid(class wiregrid *grid)
{
    grid->block_point(X, Y);
}

bool model_zero::intersect(float _X, float _Y)
{
    if (_X >= X  && _Y >= Y  && _X < X + 1 && _Y <= Y + sizeY)
        return true;
    return false;
}
