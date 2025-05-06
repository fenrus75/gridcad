
#include "gridcad.h"

wire::wire(int x1, int y1, int x2, int y2)
{
    X1 = x1;
    Y1 = y1;
    X2 = x2;
    Y2 = y2;
    
    points = NULL;
}

wire::~wire(void)
{
    if (points)
        delete points;
}

void wire::draw(class scene *scene)
{
    if (!points)
        route(scene);
}

void wire::route(class scene *scene)
{
    if (points)
        delete points;
        
    class wiregrid * grid = new wiregrid(scene->sizeX, scene->sizeY);
    
    scene->fill_grid(grid);
}
