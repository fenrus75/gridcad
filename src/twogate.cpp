#include "gridcad.h"

#include <SDL2/SDL_image.h>

twogate::twogate(float _X, float _Y) : element(3, 3, NULL)
{
    X = _X;
    Y = _Y;
    visual = NULL;
    ports.resize(0);
    
    add_port(0, 0, "In1");
    add_port(0, 2, "In2");
    add_port(2, 1, "Out");
}

twogate::~twogate(void)
{
}


void twogate::draw(class scene *scene, int type)
{
    if (!visual)
       visual =  scene->load_image("assets/andgate.png");
        
    scene->draw_image(visual, X, Y, 3, 3);

//    for (auto port: ports) {
//        drawConnector(scene, X, Y, port->X, port->Y, COLOR_ELEMENT_CONNECTOR + type);    
//    }
    for (auto wire : wires) {
        wire->draw(scene);
    }
    
}