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

void element::drawConnector(class scene *scene, float X, float Y, int cX, int cY, int type)
{
    scene->drawCircle(cX + X, cY+ Y, 0.25, type);
}

void element::drawAt(class scene *scene, float X, float Y, int type)
{
    assert (type <= DRAW_ORIGIN);
    
    scene->drawBox(X + 0.25, Y + 0.25, X + sizeX - 0.25, Y + sizeY - 0.25, COLOR_ELEMENT_NORMAL + type);
    scene->drawBox(X + 0.45, Y + 0.45, X + sizeX - 0.45, Y + sizeY - 0.45, COLOR_ELEMENT_INSIDE);
    
    /* TODO: real connectors */
    drawConnector(scene, X, Y, 0, 0, COLOR_ELEMENT_CONNECTOR + type);    
    drawConnector(scene, X, Y, 0, 1, COLOR_ELEMENT_CONNECTOR + type);    
    drawConnector(scene, X, Y, 0, 2, COLOR_ELEMENT_CONNECTOR + type);    
    drawConnector(scene, X, Y, 0, 3, COLOR_ELEMENT_CONNECTOR + type);    
    drawConnector(scene, X, Y, 1, 0, COLOR_ELEMENT_CONNECTOR + type);    
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
        case DRAW_ORIGIN:
            drawAt(scene, X, Y, DRAW_ORIGIN);
            break;
            
        default:
            assert(0);
        
    }
}