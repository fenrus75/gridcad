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
#include "contextmenu.h"

#include <map>
#include <SDL2/SDL_image.h>


void callback_ib(class iconbar *bar, class oneiconbar *choice)
{
    bar->set(choice);
}

iconbar::iconbar(SDL_Renderer *_renderer, SDL_Rect _rect)
{
    renderer = _renderer;
    rect = _rect;
    
    create_menu();    
}

void oneiconbar::set_element(unsigned int X, unsigned int Y, std::string class_id, std::string filename, std::string tooltip, struct library_block *block)
{
     icons[X][Y] = new icon(renderer, class_id, filename, tooltip);
     cIndex = Y * 2 + X + 1;
     
     if (Y >= icons[X].size())
        assert(0);
     if (class_id == "library")
          icons[X][Y]->assign_library_element(*block);
}

void oneiconbar::set_element(std::string class_id, struct library_block *block)
{
    set_element(cIndex % 2, cIndex/2, class_id, "", "",block);
}
void oneiconbar::set_element(std::string class_id, std::string filename, std::string tooltip, struct library_block *block)
{
    set_element(cIndex % 2, cIndex/2, class_id, filename, tooltip, block);
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
#define OFFSETX 0
#define OFFSETY 20

void iconbar::draw(void)
{
    SDL_Rect drop;
    SDL_RenderSetClipRect(renderer, &rect);    
    int w, h;
    float newH;

    if (dropdown == NULL)
       dropdown = IMG_LoadTextureFromMem(renderer, "assets/icondropdown.png");     
    
    if (current_icons)
         current_icons->draw();
         
    if (!dropdown)
      return;
         
    drop.x = rect.x + 5;
    drop.w = rect.w - 10;
    drop.y = 5;
    SDL_QueryTexture(dropdown, NULL, NULL, &w, &h);
    
    newH = 1.0 * h/w * drop.w;
    drop.h = newH;
    
    SDL_SetTextureAlphaMod(dropdown, 255);
    SDL_RenderCopy(renderer, dropdown, NULL, &drop);
}

class icon * iconbar::current_icon(int ScreenX, int ScreenY)
{
    return current_icons->current_icon(ScreenX, ScreenY);
}

class contextmenu * iconbar::get_menu(int ScreenX, int ScreenY)
{
     if (ScreenY < OFFSETY * 1.5) {
      return menu;
    }
    return NULL;
}

std::string iconbar::current_tooltip(int ScreenX, int ScreenY)
{
    return current_icons->current_tooltip(ScreenX, ScreenY);
}

void iconbar::handle_event(SDL_Event &event)
{
     return;
}


icon::icon(SDL_Renderer *renderer, std::string _class_id, std::string filename, std::string _tooltip)
{
    _renderer = renderer;
    class_id = _class_id;
    tooltip = _tooltip;
    active = false;
    if (filename != "")
       texture = IMG_LoadTextureFromMem(renderer, filename.c_str());
}

icon::~icon(void)
{
}

void icon::assign_library_element(struct library_block block) 
{ 
  lib = block;
  texture = IMG_LoadTextureFromMem(_renderer, lib.icon.c_str() );
  tooltip = block.tooltip;
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
     element->set_library_origin(lib.collection, lib.name);
     return element;
}

oneiconbar::oneiconbar(SDL_Renderer *_renderer, SDL_Rect _rect, std::string _name)
{
   renderer = _renderer;
   rect = _rect;
    icons.resize(2);
    
    icons[0].resize(15);
    icons[1].resize(15);
    
    name = _name;
}

oneiconbar::~oneiconbar(void)
{
      unsigned int y,x;
//      printf("oneiconbar destructor\n");
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
                box.x = rect.x + (0.5 + 2.5 * x) * SCALEX + OFFSETX;
                box.y = rect.y + (0.5 + 2.5 * y) * SCALEY + OFFSETY;
                box.h = 2 * SCALEY;
                box.w = 2 * SCALEX;
        	SDL_SetRenderDrawColor(renderer, R(color), G(color), B(color),
			       Alpha(color));
        	SDL_RenderFillRect(renderer, &box);
                if (icons[x][y])
                   icons[x][y]->draw(renderer, rect.x + (0.6 + 2.5 * x) * SCALEX + OFFSETX, rect.y + (0.6 + 2.5 * y) * SCALEY + OFFSETY, 1.8 * SCALEX, 1.8 * SCALEY);
                
                
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
      
    dX = (ScreenX - OFFSETX) / (2.5 * SCALEX);
    dY = (ScreenY - OFFSETY)/ (2.5 * SCALEY);
    
    gridX = floor(dX);
    gridY = floor(dY);
    
    if (gridX < 0)
        return NULL;
    if (gridY < 0)
        return NULL;    
    if (gridX >= (int)icons.size())
        return NULL;
    if (gridY >= (int)icons[gridX].size())
        return NULL;
        
    if (icons[gridX][gridY])
       icons[gridX][gridY]->set_active();
        
    return icons[gridX][gridY];
    
}

std::string oneiconbar::current_tooltip(int ScreenX, int ScreenY)
{
    float dX, dY;
    int gridX, gridY;
    
    
    ScreenX -= rect.x;
    ScreenY -= rect.y;
    
    /* subtract the border */
    ScreenX -= 0.5 * SCALEX;
    ScreenY -= 0.5 * SCALEY;
    
    if (ScreenX < 0 || ScreenY <0) /* left of the icons */
      return "";
      
    dX = (ScreenX - OFFSETX) / (2.5 * SCALEX);
    dY = (ScreenY - OFFSETY) / (2.5 * SCALEY);
    
    gridX = floor(dX);
    gridY = floor(dY);
    if (gridX < 0)
        return "";
    if (gridY < 0)
        return "";    
    
    if (gridX >= (int)icons.size())
        return "";
    if (gridY >= (int)icons[gridX].size())
        return "";

     if (icons[gridX][gridY])
        return icons[gridX][gridY]->get_tooltip();
    return "";
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


void iconbar::set(class oneiconbar *b)
{
     current_icons = b;
     for (unsigned int i = 0; i < icons.size(); i++) {
         if (icons[i] == b)
            active_index = i;
     }
}

void iconbar::create_menu(void)
{
     unsigned int li;
    
    class oneiconbar *init_icon, *libtab;
   if (menu) {
     delete menu;
     menu = NULL;
    }
    
    for (auto I : icons) {
        delete I;
    }
    icons.clear();
    libtabs.clear();
    
    init_icon = new class oneiconbar(renderer, rect, "Logic");

    init_icon->set_element("model_zero:", "assets/zero.png", "constant zero value");
    init_icon->set_element("model_one:", "assets/one.png", "constant one value");
    init_icon->set_element("model_not:", "assets/inverter.png", "bit inverter");
    init_icon->set_element("model_buffer:", "assets/buffer.png", "buffer with Z-state output");
    init_icon->set_element("model_nand:", "assets/nandgate.png", "bitwise NAND gate");
    init_icon->set_element("model_and:", "assets/andgate.png", "bitwise AND gate");
    init_icon->set_element("model_nor:", "assets/norgate.png", "bitwise NOR gate");
    init_icon->set_element("model_or:", "assets/orgate.png", "bitwise OR gate");
    init_icon->set_element("model_xor:", "assets/xorgate.png", "bitwise XOR gate");
    init_icon->insert_blank();
    init_icon->set_element("model_toggle:", "assets/toggle_off.png", "Input element");
    init_icon->set_element("model_output:", "assets/output_on.png", "Output element");
    init_icon->set_element("model_nest:", "assets/nest/nest_icon.png", "Nested scene");
    init_icon->set_element("model_truth:", "assets/model_truth/truthtable_icon.png", "Truth table");
    init_icon->set_element("model_dflipflop:", "assets/dflipflop.png", "D flip flop");
    init_icon->set_element("model_deflipflop:", "assets/deflipflop.png", "D flip flop with enable signal");
    init_icon->set_element("model_clock:", "assets/clock_on.png", "Clock");
    init_icon->set_element("model_datascope:", "assets/datascope.png", "Data scope");
    init_icon->set_element("model_label:", "assets/label.png", "Area Label");

    icons.push_back(init_icon);
    current_icons = init_icon;    
    init_icon = new class oneiconbar(renderer, rect, "Bus converters");    
    init_icon->set_element("model_4to1:", "assets/4to1.png", "Bus to 4 bits split");
    init_icon->set_element("model_1to4:", "assets/1to4.png", "4 bits to Bus concentrator");
    init_icon->set_element("model_8to1:", "assets/8to1.png", "Bus to 8 bits split");
    init_icon->set_element("model_1to8:", "assets/1to8.png", "8 bits to Bus concentrator");
    init_icon->set_element("model_8to4:", "assets/8to4.png", "bus8 to 2 bus4 split");
    init_icon->set_element("model_4to8:", "assets/4to8.png", "2 bus4 to bus8 concentrator");
    init_icon->set_element("model_16to8:", "assets/16to8.png", "bus16 to 2 bus8 split");
    init_icon->set_element("model_8to16:", "assets/8to16.png", "2 bus8 to bus16 concentrator");
    icons.push_back(init_icon);

    init_icon = new class oneiconbar(renderer, rect, "Memory");
    init_icon->set_element("model_memory:", "assets/memory.png", "8 Bit Memory");
    init_icon->insert_blank();
    init_icon->set_element("model_clock:", "assets/clock_on.png", "Clock");
    init_icon->set_element("model_dflipflop:", "assets/dflipflop.png", "D flip flop");
    icons.push_back(init_icon);
    

    for (li = 0; li < library.size(); li++) {
        if (libtabs.find(library[li].collection) == libtabs.end()) {
            libtab = new class oneiconbar(renderer, rect, library[li].collection );
            libtabs[library[li].collection] = libtab;
            icons.push_back(libtab);
        }
        libtab = libtabs[library[li].collection];
        libtab->set_element("library", &library[li]);
    }

    menu = new icon_contextmenu(this);
    for (auto ib : icons) {
       menu->add_item(ib->get_name(), ib, callback_ib);
    }

}