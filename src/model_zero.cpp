#include "gridcad.h"
#include "models.h"

model_zero::model_zero(float _X, float _Y)  : element(3, 3, NULL)
{
    sizeX = 2;
    sizeY = 1;    
    X = floorf(_X);
    Y = floorf(_Y);

    add_port(1, 0, "ZERO", PORT_OUT);    
}

model_zero::~model_zero(void)
{
}

void model_zero::draw(class scene *scene, int type)
{
    scene->drawBox(X,Y,X+1,Y+1, COLOR_ELEMENT_NORMAL);
    scene->drawCircle(X+1.5, Y+0.5, 0.5, COLOR_WIRE_SOLID);
    for (auto wire : wires) {
        wire->draw(scene);
    }
}

void model_zero::fill_grid(class wiregrid *grid)
{
    grid->block_point(X, Y);
    grid->block_point(X+1, Y);
}
