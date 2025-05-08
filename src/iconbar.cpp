#include "gridcad.h"
#include "iconbar.h"


iconbar::iconbar(SDL_Renderer *_renderer, SDL_Rect _rect)
{
    icons.resize(2);
    
    icons[0].resize(10);
    icons[1].resize(10);
        
    renderer = _renderer;
    rect = _rect;
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
            printf("size %i x %i \n", icons.size(), x);
            if (icons[x][y])
                icons[x][y]->draw(renderer, rect.x + (0.5 + 2.5 * x) * SCALEX, rect.y + (0.5 + 2.5 * y) * SCALEY);
            else {
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

void icon::draw(SDL_Renderer *renderer, int X, int Y)
{
}