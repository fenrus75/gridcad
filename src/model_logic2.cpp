#include "gridcad.h"
#include "model_logic2.h"
#include "port.h"

#include <sys/time.h>
#include <math.h>
model_logic2::model_logic2(float _X, float _Y, std::string _filename)  : element(_X, _Y, "")
{
    ports.resize(0);
    
    sizeX=4;
    sizeY=3;
    
    filename = _filename;
    add_port(-1, 0, "In1", PORT_IN, 1);
    add_port(-1, 2, "In2", PORT_IN, 1);
    add_port(sizeX, 1, "Out", PORT_OUT, 1);
}

model_logic2::~model_logic2(void)
{
}

void model_logic2::drawAt(class canvas *canvas, float X, float Y, int type)
{
    int alpha = Alpha(type);

    if (selected) {
        canvas->draw_image_rotated(selected_filename, X, Y, sizeX, sizeY, alpha, angle);
    } else {	
        canvas->draw_image_rotated(filename, X, Y, sizeX , sizeY, alpha, angle);
    }
    for (auto port: ports) {
        port->drawAt(canvas, X, Y, type);
    }
}


void model_logic2::calculate(int ttl)
{
}

void model_logic2::rotate_ports(void)
{
    for (auto port : ports) {
        float x,y,_x,_y;
        x = port->X - (sizeX-1)/2.0;
        y = port->Y - (sizeY-1)/2.0;
        
        _x = y;
        _y = -x;

	float tmp = sizeY;
	sizeY = sizeX;
	sizeX = tmp;

        port->X = _x + (sizeX-1)/2.0 ;
        port->Y = _y + (sizeY-1)/2.0;
        port->screenX = X + port->X;
        port->screenY = Y + port->Y;

	tmp = sizeY;
	sizeY = sizeX;
	sizeX = tmp;
        
        port->route_wires();
    }
	float tmp = sizeY;
	sizeY = sizeX;
	sizeX = tmp;
    reseat();
    angle -= 90;
    if (angle >= 360)
	angle -= 360;
    if (angle < 0)
	angle += 360;
}
