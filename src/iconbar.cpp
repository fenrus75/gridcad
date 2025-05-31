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
#include "model_nest.h"

#include <SDL2/SDL_image.h>

iconbar::iconbar(SDL_Renderer *_renderer, SDL_Rect _rect)
{
    unsigned int li;
    
    class oneiconbar *init_icon, *libtab;
    renderer = _renderer;
    rect = _rect;
    
    init_icon = new class oneiconbar(_renderer, _rect);

    init_icon->set_element("model_zero:", "assets/zero.png");
    init_icon->set_element("model_one:", "assets/one.png");
    init_icon->set_element("model_not:", "assets/inverter.png");
    init_icon->insert_blank();
    init_icon->set_element("model_nand:", "assets/nandgate.png");
    init_icon->set_element("nodel_and:", "assets/andgate.png");
    init_icon->set_element("model_nor:", "assets/norgate.png");
    init_icon->set_element("model_or:", "assets/orgate.png");
    init_icon->set_element("model_xor:", "assets/xorgate.png");
    init_icon->insert_blank();
    init_icon->set_element("model_toggle:", "assets/toggle_off.png");
    init_icon->set_element("model_output:", "assets/output_on.png");
    init_icon->set_element("model_nest:", "assets/nest/nest_icon.png");
    init_icon->set_element("model_truth:", "assets/model_truth/truthtable_icon.png");
    init_icon->set_element("model_dflipflop:", "assets/dflipflop.png");
    init_icon->set_element("model_delayline:", "assets/delayline.png");
    init_icon->set_element("model_clock:", "assets/clock_on.png");
    init_icon->set_element("model_datascope:", "assets/datascope.png");

    icons.push_back(init_icon);
    current_icons = init_icon;    
    init_icon = new class oneiconbar(_renderer, _rect);    
    init_icon->set_element("model_4to1:", "assets/4to1.png");
    init_icon->set_element("model_1to4:", "assets/1to4.png");
    init_icon->set_element("model_8to4:", "assets/8to4.png");
    init_icon->set_element("model_4to8:", "assets/4to8.png");
    icons.push_back(init_icon);

    
    libtab = new class oneiconbar(_renderer, _rect);
    
    for (li = 0; li < library.size(); li++) {
        libtab->set_element("library", &library[li]);
    }
    
    icons.push_back(libtab);
    
}

void oneiconbar::set_element(unsigned int X, unsigned int Y, std::string class_id, std::string filename, struct library_block *block)
{
     icons[X][Y] = new icon(renderer, class_id, filename);
     cIndex = Y * 2 + X + 1;
     
     if (Y >= icons[X].size())
        assert(0);
     if (class_id == "library")
          icons[X][Y]->assign_library_element(*block);
}

void oneiconbar::set_element(std::string class_id, struct library_block *block)
{
    set_element(cIndex % 2, cIndex/2, class_id, "", block);
}
void oneiconbar::set_element(std::string class_id, std::string filename, struct library_block *block)
{
    set_element(cIndex % 2, cIndex/2, class_id, filename, block);
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


icon::icon(SDL_Renderer *renderer, std::string _class_id, std::string filename)
{
    _renderer = renderer;
    class_id = _class_id;
    active = false;
    if (filename != "")
       texture = IMG_LoadTexture(renderer, filename.c_str());
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
    if (class_id == "library")
        return library_element();
    return element_from_class_id(class_id);
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

