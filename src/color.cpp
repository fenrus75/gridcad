/*
 * gridcad -- a tool to do digital circuit design with a GUI
 *
 * (C) Copyright 2025  -- Arjan van de Ven
 *
 * Licensed under the terms of the
 * GNU General Public LIcense version 3.0
 *
 * Authors:
 *    Arjan van de Ven <arjanvandeven@gmail.com>
 */

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
 {  255,  200,   32, 64},		// COLOR_BACKGROUND_YELLOW
 {0x8d, 0x8d, 0x8d, 255},		// COLOR_WIRE0
 {0xf5, 0xc2, 0x2d, 255},		// COLOR_WIRE1
 {0xe1, 0x8d, 0x92, 255},		// COLOR_WIRE2
 {0xb4, 0x15, 0x71, 255},		// COLOR_WIRE3
 {0xaf, 0x42, 0xe2, 255},		// COLOR_WIRE4
 {0x62, 0x62, 0xea, 255},		// COLOR_WIRE5
 {0x27, 0xaa, 0xc5, 255},		// COLOR_WIRE6
 {0x4c, 0xe9, 0x8f, 255},		// COLOR_WIRE7
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

std::string value_color_texture(struct value *value)
{
      if (value->is_error) {
          struct timeval tv;
          gettimeofday(&tv, NULL);
          if (tv.tv_usec > 500000)
             return "assets/wire_white.png";
          else
             return "assets/wire_black.png";
      }
      switch (value->type) {
         case VALUE_TYPE_BOOL:
              if (value->boolval)
                   return "assets/wire_green.png";
              else
                   return "assets/wire_red.png";
         default:
              return "assets/wire_green.png";
      }
}

int wire_to_color(int color)
{
	if (color >= 0 && color <= 7)
		return COLOR_WIRE0 + color;
	return COLOR_WIRE_MOTION;
}

std::string wire_color_name(int color) 
{
	switch (color) {
	case 0: 
		return "Default";
		break;
	case 1: return "Yellow";
		break;
	case 2: return "Pink";
		break;
	case 3: return "Purple";
		break;
	case 4: return "Dark Purple";
		break;
	case 5: return "Blue";
		break;
	case 6: return "Bright Blue";
		break;
	case 7: return "Green";
		break;
	default:
		return "invalid";
	}
	
}