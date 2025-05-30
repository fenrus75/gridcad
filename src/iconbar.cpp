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
#include "iconbar.h"
#include "model_toggle.h"
#include "model_not.h"
#include "model_or.h"
#include "model_xor.h"
#include "model_nor.h"
#include "model_and.h"
#include "model_nand.h"
#include "model_one.h"
#include "model_zero.h"
#include "model_output.h"
#include "model_truth.h"
#include "model_nest.h"
#include "model_4to1.h"
#include "model_1to4.h"
#include "model_8to4.h"
#include "model_4to8.h"
#include "model_datascope.h"
#include "model_dflipflop.h"
#include "model_delayline.h"
#include "model_clock.h"

#include <SDL2/SDL_image.h>

iconbar::iconbar(SDL_Renderer *_renderer, SDL_Rect _rect)
{
    unsigned int li;
    
    class oneiconbar *init_icon, *libtab;
    renderer = _renderer;
    rect = _rect;
    
    init_icon = new class oneiconbar(_renderer, _rect);
    
    init_icon->set_element(ICON_ZERO);
    init_icon->set_element(ICON_ONE);
    init_icon->set_element(ICON_NOT);
    init_icon->insert_blank();
    init_icon->set_element(ICON_NAND);
    init_icon->set_element(ICON_AND);
    init_icon->set_element(ICON_NOR);
    init_icon->set_element(ICON_OR);
    init_icon->set_element(ICON_XOR);
    init_icon->insert_blank();
    init_icon->set_element(ICON_TOGGLE);
    init_icon->set_element(ICON_OUTPUT);
    init_icon->set_element(ICON_NEST);
    init_icon->set_element(ICON_TRUTH);
    init_icon->set_element(ICON_DFLIPFLOP);
    init_icon->set_element(ICON_DELAYLINE);
    init_icon->set_element(ICON_CLOCK);
    init_icon->set_element(ICON_DATASCOPE);

    icons.push_back(init_icon);
    current_icons = init_icon;    
    init_icon = new class oneiconbar(_renderer, _rect);    
    init_icon->set_element(ICON_4TO1);
    init_icon->set_element(ICON_1TO4);
    init_icon->set_element(ICON_8TO4);
    init_icon->set_element(ICON_4TO8);
    icons.push_back(init_icon);

    
    libtab = new class oneiconbar(_renderer, _rect);
    
    for (li = 0; li < library.size(); li++) {
        libtab->set_element(ICON_LIBRARY, &library[li]);
    }
    
    icons.push_back(libtab);
    
}

void oneiconbar::set_element(unsigned int X, unsigned int Y, int icontype, struct library_block *block)
{
     icons[X][Y] = new icon(renderer, icontype);
     cIndex = Y * 2 + X + 1;
     
     if (Y >= icons[X].size())
        assert(0);
     if (icontype == ICON_LIBRARY)
          icons[X][Y]->assign_library_element(*block);
}

void oneiconbar::set_element(int icontype, struct library_block *block)
{
    set_element(cIndex % 2, cIndex/2, icontype, block);
}

void oneiconbar::insert_blank(void)
{
     cIndex++;
}


iconbar::~iconbar(void)
{
     for (auto one : icons) {
        delete one;
     }
}

void iconbar::resize(SDL_Rect _rect)
{
    rect = _rect;
    for (auto one : icons)
     one->resize(_rect);
}

#define SCALEX 40
#define SCALEY 40

void iconbar::draw(void)
{
    SDL_RenderSetClipRect(renderer, &rect);    

    if (current_icons)
     current_icons->draw();
}

class icon * iconbar::current_icon(int ScreenX, int ScreenY)
{
    return current_icons->current_icon(ScreenX, ScreenY);
}

void iconbar::handle_event(SDL_Event &event)
{
     return;
}

static const char *image_names[] = 
{"library images",
 "assets/zero.png",
 "assets/one.png",
 "assets/nandgate.png",
 "assets/andgate.png",
 "assets/inverter.png",
 "assets/toggle_off.png",
 "assets/norgate.png",
 "assets/orgate.png",
 "assets/output_on.png",
 "assets/xorgate.png",
 "assets/model_truth/truthtable_icon.png",
 "assets/nest/nest_icon.png",
 "assets/4to1.png",
 "assets/1to4.png",
 "assets/8to4.png",
 "assets/4to8.png",
 "assets/datascope.png",
 "assets/dflipflop.png",
 "assets/delayline.png",
 "assets/clock_on.png",
};

icon::icon(SDL_Renderer *renderer, int _type)
{
    _renderer = renderer;
    type = _type;
    active = false;
    if (type != ICON_LIBRARY)
       texture = IMG_LoadTexture(renderer, image_names[type]);
}

icon::~icon(void)
{
}

void icon::assign_library_element(struct library_block block) 
{ 
  lib = block;
  texture = IMG_LoadTexture(_renderer, lib.icon.c_str() );
};


/* X and Y are our box to draw in -- we'll draw centered to that */
void icon::draw(SDL_Renderer *renderer, float X, float Y, float width, float height)
{
       SDL_Rect rect;
       const int color = COLOR_WIRE_SOLID;
       int w,h;
       int oX = 0, oY = 0;
       float ratioX, ratioY;
       float newwidth, newheight;
       
       SDL_QueryTexture(texture, NULL, NULL, &w, &h);

       ratioX = width / w;
       ratioY = height / h;
       
       if (ratioX > ratioY)
           ratioX = ratioY;
       else
           ratioY = ratioX;
           
       newwidth = w * ratioX;
       newheight = h * ratioY;
       
       oX = (width - newwidth) / 2;
       oY = (height - newheight) / 2; 
       
       rect.x = X + oX;
       rect.y = Y + oY;
       rect.w = newwidth;
       rect.h = newheight;
      
       SDL_SetRenderDrawColor(renderer, R(color), G(color), B(color),
			       Alpha(color));
       SDL_SetTextureAlphaMod(texture, 255);
       if (active)                              
        	SDL_RenderFillRect(renderer, &rect);
       SDL_RenderCopy(renderer, texture, NULL, &rect);
}

class element * icon::create_element(void)
{
     switch (type) {
          case ICON_LIBRARY:
              return library_element();
          case ICON_NAND:
              return new model_nand(-10, -10);
          case ICON_ONE:
              return new model_one(-10, -10);
          case ICON_ZERO:
              return new model_zero(-10, -10);
          case ICON_AND:
              return new model_and(-10, -10);
          case ICON_NOT:
              return new model_not(-10, -10);
          case ICON_TOGGLE:
              return new model_toggle(-10, -10);
          case ICON_NOR:
              return new model_nor(-10, -10);
          case ICON_OR:
              return new model_or(-10, -10);
          case ICON_XOR:
              return new model_xor(-10, -10);
          case ICON_OUTPUT:
              return new model_output(-10, -10);
          case ICON_TRUTH:
              return new model_truth(-10, -10);
          case ICON_NEST:
              return new model_nest(-10, -10);
          case ICON_4TO1:
              return new model_4to1(-10,-10);
          case ICON_1TO4:
              return new model_1to4(-10,-10);
	  case ICON_8TO4:
	      return new model_8to4(-10, -10);
	  case ICON_4TO8:
	      return new model_4to8(-10, -10);
	  case ICON_DATASCOPE:
	      return new model_datascope(-10, -10);
	  case ICON_DFLIPFLOP:
	      return new model_dflipflop(-10, -10);
	  case ICON_DELAYLINE:
	      return new model_delayline(-10, -10);
	  case ICON_CLOCK:
	      return new model_clock(-10, -10);
          default:
             return NULL;
     }
}


class element *icon::library_element(void)
{
     class model_nest *element;
     
     element = new model_nest(-10, -10);
     element->load_scene_from_json(lib.logic);
     element->update_name(lib.name);
     element->set_icon(lib.icon, lib.icon_selected);
     return element;
}

oneiconbar::oneiconbar(SDL_Renderer *_renderer, SDL_Rect _rect)
{
   renderer = _renderer;
   rect = _rect;
    icons.resize(2);
    
    icons[0].resize(15);
    icons[1].resize(15);
}

oneiconbar::~oneiconbar(void)
{
      unsigned int y,x;
      printf("oneiconbar destructor\n");
      for (y = 0; y < icons.size(); y++) {
           for (x = 0; x < icons[y].size(); x++) {
              delete icons[y][x];
              icons[y][x] = NULL;
           }
      }
}

void oneiconbar::resize(SDL_Rect _rect)
{
    rect = _rect;
}

#define SCALEX 40
#define SCALEY 40

void oneiconbar::draw(void)
{
    int x,y;
    SDL_RenderSetClipRect(renderer, &rect);    
    int color = 0;
    SDL_Rect box;
    
    for (y = 0; y < 15; y++) {
        for (x = 0; x < 2; x++) {
                box.x = rect.x + (0.5 + 2.5 * x) * SCALEX;
                box.y = rect.y + (0.5 + 2.5 * y) * SCALEY;
                box.h = 2 * SCALEY;
                box.w = 2 * SCALEY;
        	SDL_SetRenderDrawColor(renderer, R(color), G(color), B(color),
			       Alpha(color));
        	SDL_RenderFillRect(renderer, &box);
                if (icons[x][y])
                   icons[x][y]->draw(renderer, rect.x + (0.6 + 2.5 * x) * SCALEX, rect.y + (0.6 + 2.5 * y) * SCALEY, 1.8 * SCALEX, 1.8 * SCALEY);
                
                
        }    
    }
}

class icon * oneiconbar::current_icon(int ScreenX, int ScreenY)
{
    float dX, dY;
    int gridX, gridY;
    
    ScreenX -= rect.x;
    ScreenY -= rect.y;
    
    /* subtract the border */
    ScreenX -= 0.5 * SCALEX;
    ScreenY -= 0.5 * SCALEY;
    
    if (ScreenX < 0 || ScreenY <0) /* left of the icons */
      return NULL;
      
    dX = ScreenX / (2.5 * SCALEX);
    dY = ScreenY / (2.5 * SCALEY);
    
    gridX = floor(dX);
    gridY = floor(dY);
    
    if (gridX >= (int)icons.size())
        return NULL;
    if (gridY >= (int)icons[gridX].size())
        return NULL;
        
    if (icons[gridX][gridY])
       icons[gridX][gridY]->set_active();
        
    return icons[gridX][gridY];
    
}


void iconbar::next(void)
{
    if (active_index + 1 < icons.size())
        active_index++;
    current_icons = icons[active_index];
}

void iconbar::previous(void)
{
    if (active_index >= 1)
        active_index--;
    current_icons = icons[active_index];
}

