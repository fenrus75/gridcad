#pragma once

#include "gridcad.h"

class truthcanvas : public basecanvas
{
public:
    truthcanvas(class model_truth *element);
    virtual ~truthcanvas();
    void draw(void) override;
    bool handleEvent(SDL_Event &event) override;
    
private:
    class model_truth *element;
};


