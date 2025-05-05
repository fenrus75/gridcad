#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <stdio.h>

#include "gridcad.h"


scene::scene(void)
{
    window = SDL_CreateWindow("FOO BAR", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1024, 768, 0);
    renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    
    offsetX = 0;
    offsetY = 0;
    scaleX = 25;
    scaleY = 25;
    
    sizeX = 200;
    sizeY = 200;
    dragging = NULL;
    left_mouse_down = false;
    connector_img = IMG_LoadTexture(renderer, "assets/connector.png");

}

scene::~scene(void)
{
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}


void scene::eventloop(void)
{
    SDL_Event event;
    int ret = 0;
    bool leave = false;
    
    
    while (!leave) {
        if (!ret)
            SDL_Delay(1);
        ret = SDL_PollEvent(&event);
        
        switch (event.type) {
            case SDL_QUIT:
                leave = true;
                break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_UP:
                        offsetY --;
                        if  (offsetY < -1)
                            offsetY = -1;
                        break;
                    case SDLK_DOWN:
                        offsetY ++;
                        if (offsetY > sizeY + 1)
                            offsetY = sizeY;
                        break;
                    case SDLK_LEFT:
                        offsetX --;
                        if  (offsetX < -1)
                            offsetX = -1;
                        break;
                    case SDLK_RIGHT:
                        offsetX ++;
                        if (offsetX > sizeX + 1)
                            offsetX = sizeX;
                        break;
                    case SDLK_PLUS:
                    case SDLK_KP_PLUS:
                    case SDLK_EQUALS:
                        scaleX++;
                        scaleY++;
                        break;
                    case SDLK_MINUS:
                    case SDLK_KP_MINUS:
                        if (scaleX > 1)
                            scaleX--;
                        if (scaleY > 1)
                            scaleY--;
                        break;
                }
                break;
            case SDL_MOUSEBUTTONDOWN:
                if (!left_mouse_down && event.button.button == SDL_BUTTON_LEFT)  {
                    dragging = NULL;
                    left_mouse_down = true;
                    float x, y;
                     
                    x = scr_to_X(event.motion.x);
                    y = scr_to_Y(event.motion.y);
                     
                    for (auto elem : elements) {
                        if (elem->intersect(x, y))
                            dragging = elem;
                    }
                    if (dragging)
                        dragging->start_drag(x, y);
                }
                break;
            case SDL_MOUSEBUTTONUP:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    if (dragging)
                        dragging->stop_drag();
                    dragging = NULL;
                    left_mouse_down = false;
                }
                break;
            case SDL_MOUSEMOTION:
                if (dragging) {
                    dragging->update_drag(this, scr_to_X(event.motion.x), scr_to_Y(event.motion.y));
                }
        }
        
        draw();
    }
}

void scene::draw(void)
{
    /* first, draw the lighter gray background */
    SDL_SetRenderDrawColor(renderer, R(COLOR_BACKGROUND_GRID), G(COLOR_BACKGROUND_GRID), B(COLOR_BACKGROUND_GRID), Alpha(COLOR_BACKGROUND_GRID));
    SDL_RenderClear(renderer);
    
    /* then fill in the darger gray grid to keep lines */
    float X = 0, Y = 0;
    while (Y < sizeY) {
        X = 0;
        while (X < sizeX) {
            drawBox(X + 0.2, Y + 0.2, X + 9.8, Y + 9.8, COLOR_BACKGROUND_MAIN);
            X = X + 10;
        }
        Y = Y + 10;
    }
    
    /* draw the wires before the elements */
    
    /* draw the elements */
    for (auto const elem : elements) {
        if (elem != dragging)
            elem->draw(this, DRAW_NORMAL);
        else
            elem->draw(this, DRAW_ORIGIN);
    }
    
    if (dragging) {
        dragging->draw(this, DRAW_GHOST);
        dragging->draw(this, DRAW_DND);
    }

    SDL_RenderPresent(renderer);     
}

int scene::X_to_scr(float X)
{
    return (X - offsetX) * scaleX;
}

float scene::scr_to_X(int X)
{
    return X / scaleX + offsetX;
}

int scene::Y_to_scr(float Y)
{
    return (Y - offsetY) * scaleY;
}

float scene::scr_to_Y(int Y)
{
    return Y / scaleY + offsetY;
}

void scene::drawBox(float X1, float Y1, float X2, float Y2, int color)
{
    int x1,y1,x2,y2;
    
    SDL_Rect rect;
    
    x1 = X_to_scr(X1);
    x2 = X_to_scr(X2);
    y1 = Y_to_scr(Y1);
    y2 = Y_to_scr(Y2);
    
    /* clip */
    if (x1 < 0)
        x1 = 0;
    if (x2 < 0)
        return;
    if (y1 < 0)
        y1 = 0;
    if (y2 < 0)
        return;
        
    /* TODO: clip boxes that are fully outside of the visible window */
        
        
    rect.x = x1;
    rect.y = y1;
    rect.w = x2 - x1;
    rect.h = y2 - y1;
    
    SDL_SetRenderDrawColor(renderer, R(color), G(color), B(color), Alpha(color));
    SDL_RenderFillRect(renderer, &rect);    
}

void scene::drawCircle(float X, float Y, float _R, int color)
{
    int x1,y1,x2,y2;
    
    SDL_Rect rect;
    
    x1 = X_to_scr(X - _R);
    x2 = X_to_scr(X + _R);
    y1 = Y_to_scr(Y - _R);
    y2 = Y_to_scr(Y + _R);
    
    /* clip */
    if (x1 < 0)
        x1 = 0;
    if (x2 < 0)
        return;
    if (y1 < 0)
        y1 = 0;
    if (y2 < 0)
        return;
        
    /* TODO: clip boxes that are fully outside of the visible window */
        
        
    rect.x = x1;
    rect.y = y1;
    rect.w = x2 - x1;
    rect.h = y2 - y1;
    
    SDL_SetTextureAlphaMod(connector_img, Alpha(color));
    SDL_RenderCopy(renderer, connector_img, NULL, &rect);
    
//    SDL_SetRenderDrawColor(renderer, R(color), G(color), B(color), Alpha(color));
//    SDL_RenderFillRect(renderer, &rect);    
    
}

void scene::add_element(class element *element)
{
    elements.push_back(element);
}

bool scene::can_place_element(float x, float y, int w, int h, class element *myself)
{
    for (auto const elem : elements) {
        int _x, _y;
        if (elem == myself)
            continue;
       for (_y = -1; _y <= h+1; _y++)
           for (_x = -1; _x <=  w+1; _x++)
               if (elem->intersect(x + _x, y + _y))
                   return false;
    }
    return true;
}

struct port *scene::is_port(float X, float Y)
{
    for (auto const elem : elements) {
        struct port *p = elem->is_port(X, Y);
        if (p)
            return p;
    }
    return NULL;

}