#pragma once

#include "gridcad.h"

class element;
class twogate;

class model_zero : public element
{
public:
    model_zero(float X, float Y);
    virtual ~model_zero(void);
    virtual void drawAt(class scene *scene, float X, float Y, int type);
    virtual void fill_grid(class wiregrid* grid);
    virtual bool intersect(float _X, float _Y);
};


class model_one : public model_zero
{
public:
    model_one(float X, float Y);
    virtual ~model_one(void);
    virtual void drawAt(class scene *scene, float X, float Y, int type);
};


class model_nand : public element
{
public:
    model_nand(float X, float Y);
    virtual ~model_nand(void);
    virtual void drawAt(class scene *scene, float X, float Y, int type);
protected:
    SDL_Texture *visual = NULL;
};