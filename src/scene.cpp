#include <SDL2/SDL.h>
#include <stdio.h>

#include "gridcad.h"


scene::scene(void)
{
    window = SDL_CreateWindow("FOO BAR", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1024, 768, 0);
    renderer = SDL_CreateRenderer(window, -1, 0);
    
    offsetX = 0;
    offsetY = 0;
    scaleX = 25;
    scaleY = 25;
    
    sizeX = 200;
    sizeY = 200;
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
        }
        
        draw();
    }
}

void scene::draw(void)
{
    /* first, draw the lighter gray background */
    SDL_SetRenderDrawColor(renderer, 120, 120, 120, 255);
    SDL_RenderClear(renderer);
    
    /* then fill in the darger gray grid to keep lines */
    float X = 0, Y = 0;
    while (Y < sizeY) {
        X = 0;
        while (X < sizeX) {
            drawBox(X + 0.2, Y + 0.2, X + 9.8, Y + 9.8);
            X = X + 10;
        }
        Y = Y + 10;
    }

    SDL_RenderPresent(renderer);     
}

int scene::X_to_scr(float X)
{
    return (X - offsetX) * scaleX;
}

int scene::Y_to_scr(float Y)
{
    return (Y - offsetY) * scaleY;
}

void scene::drawBox(float X1, float Y1, float X2, float Y2)
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
        
        
    rect.x = x1;
    rect.y = y1;
    rect.w = x2 - x1;
    rect.h = y2 - y1;
    
    SDL_SetRenderDrawColor(renderer, 42, 42, 42, 255);
    SDL_RenderFillRect(renderer, &rect);    
}