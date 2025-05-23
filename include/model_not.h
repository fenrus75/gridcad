#pragma once

#include "gridcad.h"


class model_not : public element
{
public:
    model_not(float X, float Y);
    virtual ~model_not(void);
    virtual std::string class_id(void) { return "model_not:";};
    
    void drawAt(class canvas *canvas, float X, float Y, int type) override;
    
    void calculate(int ttl) override;
protected:
    SDL_Texture *visual = NULL;
    SDL_Texture *visual_selected = NULL;
};

