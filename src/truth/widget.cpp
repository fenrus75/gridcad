#include "gridcad.h"
#include "widget.h"


widget::widget(float X, float Y, float W, float H)
{
    X1 = X;
    Y1 = Y;
    X2 = X + W;
    Y2 = Y + H;
}

widget::~widget(void)
{
}