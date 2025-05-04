#include "gridcad.h"

element::element(int _sizeX, int _sizeY)
{
    sizeX = _sizeX;
    sizeY = _sizeY;
    place(0,0);
}

void element::place(int _X, int _Y)
{
    X = _X; Xghost = _X; Xdnd = _X;
    Y = _Y; Yghost = _Y; Ydnd = _Y;
}