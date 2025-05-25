#include "gridcad.h"
#include "value.h"

#include <sys/time.h>

#define MAX_COLOR 32
static int colors[MAX_COLOR][4] =
{
 { 42,  42,  42, 255},		// COLOR_BACKGROUND_MAIN
 {120, 120, 120, 255},		// COLOR_BACKGROUND_GRID
 {240, 240, 240, 255},		// COLOR_ELEMENT_NORMAL
 {240, 240, 240, 32},		// COLOR_ELEMENT_GHOST
 {160, 160, 160, 128},		// COLOR_ELEMENT_DND
 {180, 180, 180, 128},		// COLOR_ELEMENT_ORIGIN
 {0x8d, 0x8d, 0x8d, 255},		// COLOR_WRIRE_SOLID
 {255, 128, 128, 128},		// COLOR_WRIRE_INVALID
 {0x8d, 0x8d, 0x8d, 255},		// COLOR_WRIRE_MOTION
 {255,  64,  64, 255},		// COLOR_VALUE_RED
 {64,  255,  64, 255},		// COLOR_VALUE_GREEN
 {255, 255, 255, 255},		// COLOR_ERROR_WHITE
 {  0,   0,   0, 255},		// COLOR_ERROR_BLACK
 {  170,  170,   255, 64},		// COLOR_AREA_SELECT
 {  240,  240,   128, 64},		// COLOR_BACKGROUND_YELLOW
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

int value_color(struct value *value)
{
      if (value->is_error) {
          struct timeval tv;
          gettimeofday(&tv, NULL);
          if (tv.tv_usec > 500000)
             return COLOR_ERROR_WHITE;
          else
             return COLOR_ERROR_BLACK;
      }
      switch (value->type) {
         case VALUE_TYPE_BOOL:
              if (value->boolval)
                   return COLOR_VALUE_GREEN;
              else
                   return COLOR_VALUE_RED;
         default:
              return COLOR_BACKGROUND_MAIN;
      }
}