
#include "gridcad.h"

wire::wire(int x1, int y1, int x2, int y2, int _color)
{
    X1 = x1;
    Y1 = y1;
    X2 = x2;
    Y2 = y2;
    color = _color;
    
    points = NULL;
}

wire::~wire(void)
{
    if (points)
        delete points;
}

void wire::draw(class scene *scene, int color)
{
    int prevX, prevY;
    bool first = true;
    if (!points)
        route(scene);
        
    if (!points)
        return;
        
    for (auto point: *points) {
        if (first) {
            prevX = point.X;
            prevY = point.Y;
            first = false;
            continue;
        }
        scene->drawLine(prevX + 0.5, prevY + 0.5, point.X + 0.5, point.Y + 0.5, color);
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
