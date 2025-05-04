#include "gridcad.h"

#define MAX_COLOR 32
static int colors[MAX_COLOR][4] =
{
 { 42,  42,  42, 255},		// COLOR_BACKGROUND_MAIN
 {120, 120, 120, 255},		// COLOR_BACKGROUND_GRID
 {240, 240, 240, 255},		// COLOR_ELEMENT_NORMAL
 {160, 160, 160, 128},		// COLOR_ELEMENT_GHOST
 {160, 160, 160, 128},		// COLOR_ELEMENT_DND
 {240, 240, 240, 128},		// COLOR_ELEMENT_ORIGIN
};

int R(int color)
{
    if (color >= MAX_COLOR)
        return 255;
    return colors[color][0];
}

int G(int color)
{
    if (color >= MAX_COLOR)
        return 255;
    return colors[color][1];
}

int B(int color)
{
    if (color >= MAX_COLOR)
        return 255;
    return colors[color][2];
}

int Alpha(int color)
{
    if (color >= MAX_COLOR)
        return 255;
    return colors[color][3];
}

