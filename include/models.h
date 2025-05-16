#pragma once

#include "gridcad.h"

class element;
class twogate;

class model_zero : public element
{
public:
    model_zero(float X, float Y);
    virtual ~model_zero(void);
    virtual const char *class_id(void) { return "model_zero:";};
    
    void drawAt(class canvas *canvas, float X, float Y, int type) override;
    void fill_grid(class wiregrid* grid) override;
    bool intersect(float _X, float _Y) override;
};


class model_one : public model_zero
{
public:
    model_one(float X, float Y);
    virtual ~model_one(void);
    virtual const char *class_id(void) { return "model_one:";};

    void drawAt(class canvas *canvas, float X, float Y, int type) override;
};

class model_toggle : public element
{
public:
    model_toggle(float X, float Y);
    virtual ~model_toggle(void);
    virtual const char *class_id(void) { return "model_toggle:";};


    void drawAt(class canvas *canvas, float X, float Y, int type) override;
    void mouse_select(void) override;
private:
    struct value value = {};
};


class model_logic2 : public element
{
public:
    model_logic2(float X, float Y, const char *filename);
    virtual ~model_logic2(void);
    virtual const char *class_id(void) { return "model_logic2:";};
    
    void drawAt(class canvas *canvas, float X, float Y, int type) override;
    
    void calculate(int ttl) override;
protected:
    SDL_Texture *visual = NULL;
    const char *filename;
};

class model_not : public element
{
public:
    model_not(float X, float Y);
    virtual ~model_not(void);
    virtual const char *class_id(void) { return "model_not:";};
    
    void drawAt(class canvas *canvas, float X, float Y, int type) override;
    
    void calculate(int ttl) override;
protected:
    SDL_Texture *visual = NULL;
    const char *filename;
};

class model_nand : public model_logic2
{
public:
    model_nand(float X, float Y);
    virtual ~model_nand(void);
    virtual const char *class_id(void) { return "model_nand:";};
    
    void calculate(int ttl) override;
};

class model_and : public model_logic2
{
public:
    model_and(float X, float Y);
    virtual ~model_and(void);
    virtual const char *class_id(void) { return "model_and:";};
    
    void calculate(int ttl) override;
};

class model_nor : public model_logic2
{
public:
    model_nor(float X, float Y);
    virtual ~model_nor(void);
    virtual const char *class_id(void) { return "model_nor:";};    
    void calculate(int ttl) override;
};

class model_or : public model_logic2
{
public:
    model_or(float X, float Y);
    virtual ~model_or(void);
    virtual const char *class_id(void) { return "model_or:";};    
    void calculate(int ttl) override;
};
