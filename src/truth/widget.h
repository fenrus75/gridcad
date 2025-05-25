#pragma once

#include "gridcad.h"
#include "model_truth.h"
#include "truthcanvas.h"

class widget {
public:
    widget(float X, float Y, float W, float H);
    virtual ~widget();
    
    virtual void draw(class truthcanvas *canvas) = 0;
    virtual void handle_event(SDL_Event &event) = 0;
    
    virtual void select(void) { _selected = true; };
    void deselect(void) { _selected = false; };
    bool selected(void) { return _selected; };
    
    virtual bool intersect(float X, float Y) { return (X>= X1 && X <= X2 && Y >= Y1 && Y <= Y2);};
    int get_gridX(void) { return gridX; };
    int get_gridY(void) { return gridY; };
    
protected:
    float X1 = 0.0, Y1 = 0.0, X2 = 0.0, Y2 = 0.0;
    bool _selected = false;
    int gridX = -5, gridY = -5;
};

class label : public widget {
public:
    label(float X, float Y, float W, float H, std::string text);
    virtual ~label(void);

    void draw(class truthcanvas *canvas) override;
    void handle_event(SDL_Event &event) override;
    bool intersect(float X, float Y) override { return false;};
private:
    std::string text;
};

class tristate : public widget {
public:
    tristate(float X, float Y, float W, float H, char *tv, int x, int y, bool is_input);
    virtual ~tristate(void);

    void draw(class truthcanvas *canvas) override;
    void handle_event(SDL_Event &event) override;
    
private:
    char *tv = NULL;
    bool is_input;
};

#define ACTION_ADD_INPUT 1
#define ACTION_DEL_INPUT 2
#define ACTION_ADD_OUTPUT 3
#define ACTION_DEL_OUTPUT 4

class button : public widget {
public:
    button(float X, float Y, float W, float H, std::string icon, int action, class truthcanvas *canvas);
    virtual ~button(void);

    void draw(class truthcanvas *canvas) override;
    void handle_event(SDL_Event &event) override;
    void select(void) override;
private:
    int action = 0;
    std::string icon = "";
    class truthcanvas *canvas;
};
