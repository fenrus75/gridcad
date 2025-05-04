#include <assert.h>

#include "gridcad.h"

element::element(int _sizeX, int _sizeY)
{
    assert(_sizeX >= 2);
    assert(_sizeY >= 2);
    sizeX = _sizeX;
    sizeY = _sizeY;
    place(0,0);
}

void element::place(int _X, int _Y)
{
    X = _X; Xghost = _X; Xdnd = _X;
    Y = _Y; Yghost = _Y; Ydnd = _Y;
}

void element::drawAt(class scene *scene, float X, float Y, int type)
{
    assert (type <= DRAW_DND);
    
    scene->drawBox(X + 0.5, Y + 0.5, X + sizeX - 0.5, Y + sizeY - 0.5, COLOR_ELEMENT_NORMAL + type);
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
            
        default:
            assert(0);
        
    }
}