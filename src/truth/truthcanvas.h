#pragma once

#include "gridcad.h"

#include "widget.h"

class truthcanvas : public basecanvas
{
public:
    truthcanvas(class model_truth *element);
    virtual ~truthcanvas();
    void draw(void) override;
    bool handleEvent(SDL_Event &event) override;
    void add_widget(class widget *widget);
    
private:
    class model_truth *element;
    std::vector<class widget *> widgets;
};


