#pragma once

#include "gridcad.h"
#include "model_truth.h"

class widget {
public:
    widget(float X, float Y, float W, float H);
    virtual ~widget();
    
    virtual void draw(class basecanvas *canvas) = 0;
    virtual void handle_event(SDL_Event &event) = 0;
    
    void select(void) { _selected = true; };
    void deselect(void) { _selected = false; };
    bool selected(void) { return _selected; };
    
    virtual bool intersect(float X, float Y) { return (X>= X1 && X <= X2 && Y >= Y1 && Y <= Y2);};
    int get_gridX(void) { return gridX; };
    int get_gridY(void) { return gridY; };
    
protected:
    float X1 = 0.0, Y1 = 0.0, X2 = 0.0, Y2 = 0.0;
    bool _selected = false;
    int gridX = 0, gridY = 0;
};

class label : public widget {
public:
    label(float X, float Y, float W, float H, std::string text);
    virtual ~label(void);

    void draw(class basecanvas *canvas) override;
    void handle_event(SDL_Event &event) override;
    bool intersect(float X, float Y) override { return false;};
private:
    std::string text;
};

class tristate : public widget {
public:
    tristate(float X, float Y, float W, float H, struct truthvalue *tv, int x, int y);
    virtual ~tristate(void);

    void draw(class basecanvas *canvas) override;
    void handle_event(SDL_Event &event) override;
    
private:
    struct truthvalue *tv = NULL;
};


