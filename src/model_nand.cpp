#include "gridcad.h"
#include "models.h"

model_nand::model_nand(float _X, float _Y)  : element(_X, _Y, NULL)
{
    ports.resize(0);
    
    sizeX=5;
    sizeY=3;
    
    add_port(0, 0, "In1", PORT_IN);
    add_port(0, 2, "In2", PORT_IN);
    add_port(sizeX, 1, "Out", PORT_OUT);
}

model_nand::~model_nand(void)
{
}

void model_nand::drawAt(class scene *scene, float X, float Y, int type)
{
    if (!visual) {
        printf("LOAD\n");
        visual = scene->load_image("assets/nandgate.png");    
    }
    scene->draw_image(visual, X+1, Y, sizeX-1, sizeY, Alpha(type));
    for (auto port: ports) {
        port->drawAt(scene, X, Y, type);
    }
}


void model_nand::calculate(void)
{
    struct value result = {};
    
    result.type = VALUE_TYPE_BOOL;
    result.boolval = !(ports[0]->value.boolval && ports[1]->value.boolval);
    
    ports[2]->update_value(&result);
    
}