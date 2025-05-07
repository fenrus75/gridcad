#include "gridcad.h"

#include <SDL2/SDL_image.h>

twogate::twogate(float _X, float _Y) : element(2, 2, NULL)
{
    X = _X;
    Y = _Y;
    visual = NULL;
    ports.resize(0);
    
    add_port(0, 0, "In1", PORT_IN);
    add_port(0, 2, "In2", PORT_IN);
    add_port(2, 1, "Out", PORT_OUT);
}

twogate::~twogate(void)
{
}


void twogate::drawAt(class scene *scene, float X, float Y, int type)
{
    if (!visual)
       visual =  scene->load_image("assets/andgate.png");
       
    
        
    scene->draw_image(visual, X, Y, 3, 3, Alpha(type));

//    for (auto port: ports) {
//        drawConnector(scene, X, Y, port->X, port->Y, COLOR_ELEMENT_CONNECTOR + type);    
//    }
    for (auto wire : wires) {
        wire->draw(scene);
    }
}

#if 0
void twogate::draw(class scene *scene, int type)
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
#endif