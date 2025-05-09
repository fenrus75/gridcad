#include "gridcad.h"
#include "iconbar.h"

#include <SDL2/SDL_image.h>

iconbar::iconbar(SDL_Renderer *_renderer, SDL_Rect _rect)
{
    icons.resize(2);
    
    icons[0].resize(10);
    icons[1].resize(10);
        
    renderer = _renderer;
    rect = _rect;
    
    icons[0][0] = new icon(renderer, ICON_NAND);
}

iconbar::~iconbar(void)
{
}

void iconbar::resize(SDL_Rect _rect)
{
    rect = _rect;
}

#define SCALEX 40
#define SCALEY 40

void iconbar::draw(void)
{
    int x,y;
    SDL_RenderSetClipRect(renderer, &rect);    
    int color = 0;
    SDL_Rect box;
    
    for (y = 0; y < 10; y++) {
        for (x = 0; x < 2; x++) {
            if (icons[x][y]) {
                icons[x][y]->draw(renderer, rect.x + (0.5 + 2.5 * x) * SCALEX, rect.y + (0.5 + 2.5 * y) * SCALEY, 2 * SCALEX, 2 * SCALEY);
            } else {
                box.x = rect.x + (0.5 + 2.5 * x) * SCALEX;
                box.y = rect.y + (0.5 + 2.5 * y) * SCALEY;
                box.h = 2 * SCALEY;
                box.w = 2 * SCALEY;
        	SDL_SetRenderDrawColor(renderer, R(color), G(color), B(color),
			       Alpha(color));
        	SDL_RenderFillRect(renderer, &box);
            }
                
        }    
    }
}

class icon * iconbar::current_icon(int ScreenX, int ScreenY)
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
    
    dX = ScreenX / 2.5;
    dY = ScreenY / 2.5;
    
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

static const char *image_names[] = 
{"",
 "",
 "assets/nandgate.png",
};

icon::icon(SDL_Renderer *renderer, int _type)
{
    type = _type;
    active = false;
    texture = IMG_LoadTexture(renderer, image_names[type]);
}

icon::~icon(void)
{
}


void icon::draw(SDL_Renderer *renderer, float X, float Y, float width, float height)
{
       SDL_Rect rect;
       const int color = COLOR_WIRE_SOLID;
       
       rect.x = X;
       rect.y = Y;
       rect.w = width;
       rect.h = height;
       SDL_SetRenderDrawColor(renderer, R(color), G(color), B(color),
			       Alpha(color));
       if (active)                              
        	SDL_RenderFillRect(renderer, &rect);
       SDL_RenderCopy(renderer, texture, NULL, &rect);
}

class element * icon::create_element(void)
{
     switch (type) {
          default:
             return NULL;
     }
}


