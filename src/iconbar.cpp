#include "gridcad.h"
#include "iconbar.h"
#include "models.h"

#include <SDL2/SDL_image.h>

iconbar::iconbar(SDL_Renderer *_renderer, SDL_Rect _rect)
{
    icons.resize(2);
    
    icons[0].resize(10);
    icons[1].resize(10);
        
    renderer = _renderer;
    rect = _rect;
    
    icons[0][0] = new icon(renderer, ICON_ZERO);
    icons[1][0] = new icon(renderer, ICON_ONE);
    icons[0][1] = new icon(renderer, ICON_NOT);
    icons[1][1] = new icon(renderer, ICON_TOGGLE);
    icons[0][2] = new icon(renderer, ICON_NAND);
    icons[1][2] = new icon(renderer, ICON_AND);
    icons[0][3] = new icon(renderer, ICON_NOR);
    icons[1][3] = new icon(renderer, ICON_OR);
    icons[0][4] = new icon(renderer, ICON_XOR);
    icons[1][5] = new icon(renderer, ICON_OUTPUT);
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

static const char *image_names[] = 
{"assets/zero.png",
 "assets/one.png",
 "assets/nandgate.png",
 "assets/andgate.png",
 "assets/inverter.png",
 "assets/toggle_off.png",
 "assets/norgate.png",
 "assets/orgate.png",
 "assets/output_on.png",
 "assets/xorgate.png",
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
          default:
             return NULL;
     }
}


