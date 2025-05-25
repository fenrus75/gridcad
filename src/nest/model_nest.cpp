#include "gridcad.h"

#include "model_nest.h"


model_nest::model_nest(float _X, float _Y) : element(_X,_Y, NULL)
{
	sizeX = 4;
	sizeY = 4;
	X = floorf(_X);
	Y = floorf(_Y);    
}

model_nest::~model_nest(void)
{
}


void model_nest::drawAt(class canvas *canvas, float X, float Y, int type)
{
	canvas->draw_image("assets/nest/nest_back.png", X, Y, sizeX, sizeY, Alpha(type));
}
