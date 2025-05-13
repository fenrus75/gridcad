#pragma once

#include "gridcad.h"

class element;
class twogate;

class model_zero : public element
{
public:
    model_zero(float X, float Y);
    virtual ~model_zero(void);
    void drawAt(class scene *scene, float X, float Y, int type) override;
    void fill_grid(class wiregrid* grid) override;
    bool intersect(float _X, float _Y) override;
};


class model_one : public model_zero
{
public:
    model_one(float X, float Y);
    virtual ~model_one(void);
    void drawAt(class scene *scene, float X, float Y, int type) override;
};

class model_toggle : public element
{
public:
    model_toggle(float X, float Y);
    virtual ~model_toggle(void);
    void drawAt(class scene *scene, float X, float Y, int type) override;
    void mouse_select(void) override;
private:
    struct value value = {};
};


class model_logic2 : public element
{
public:
    model_logic2(float X, float Y, const char *filename);
    virtual ~model_logic2(void);
    void drawAt(class scene *scene, float X, float Y, int type) override;
    
    void calculate(void) override;
protected:
    SDL_Texture *visual = NULL;
    const char *filename;
};

class model_not : public element
{
public:
    model_not(float X, float Y);
    virtual ~model_not(void);
    void drawAt(class scene *scene, float X, float Y, int type) override;
    
    void calculate(void) override;
protected:
    SDL_Texture *visual = NULL;
    const char *filename;
};

class model_nand : public model_logic2
{
public:
    model_nand(float X, float Y);
    virtual ~model_nand(void);
    
    void calculate(void) override;
};

class model_and : public model_logic2
{
public:
    model_and(float X, float Y);
    virtual ~model_and(void);
    
    void calculate(void) override;
};

class model_nor : public model_logic2
{
public:
    model_nor(float X, float Y);
    virtual ~model_nor(void);
    
    void calculate(void) override;
};

class model_or : public model_logic2
{
public:
    model_or(float X, float Y);
    virtual ~model_or(void);
    
    void calculate(void) override;
};
