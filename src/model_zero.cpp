#include "gridcad.h"
#include "model_zero.h"

model_zero::model_zero(float _X, float _Y)  : element(1, 1, "")
{
    struct value value;
    sizeX = 2;
    sizeY = 1;    
    X = floorf(_X);
    Y = floorf(_Y);

    
    memset(&value, 0, sizeof(struct value));
    value.boolval = false;
    value.valid = true;

    ports.clear();

    add_port(1, 0, "ZERO", PORT_OUT);    


    for (auto port : ports) {
        port->update_value(&value, DEFAULT_TTL);
    }

}

model_zero::~model_zero(void)
{
}

void model_zero::drawAt(class canvas *canvas, float X, float Y, int type)
{
    if (!visual) {
        visual = canvas->load_image("assets/zero.png");    
        visual_selected = canvas->load_image("assets/zero_base.png");
    }
    if (selected && visual_selected) {
        canvas->draw_image(visual_selected, X, Y, sizeX, sizeY, Alpha(type));
    } else {	
        canvas->draw_image(visual, X, Y, sizeX, sizeY, Alpha(type));
    }

    for (auto port : ports) {
        port->draw_wires(canvas);
    }
}

void model_zero::fill_grid(class wiregrid *grid)
{
    grid->block_point(X, Y);
    grid->add_soft_cost(X+1, Y - 1, 0.2);
    grid->add_soft_cost(X+1, Y + 1, 0.2);
    grid->add_soft_cost(X, Y - 1, 0.6);
    grid->add_soft_cost(X, Y + 1, 0.6);
    grid->add_soft_cost(X-1, Y - 1, 0.3);
    grid->add_soft_cost(X-1, Y,    0.3);
    grid->add_soft_cost(X-1, Y + 1, 0.3);
}

bool model_zero::intersect(float _X, float _Y)
{
    if (_X >= X  && _Y >= Y  && _X < X + 1 && _Y <= Y + sizeY)
        return true;
    return false;
}
