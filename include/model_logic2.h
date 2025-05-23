#pragma once

#include "gridcad.h"
#include "port.h"

class model_logic2 : public element
{
public:
    model_logic2(float X, float Y, const char *filename);
    virtual ~model_logic2(void);
    virtual std::string class_id(void) { return "model_logic2:";};
    
    void drawAt(class canvas *canvas, float X, float Y, int type) override;
    
    void calculate(int ttl) override;
protected:
    SDL_Texture *visual = NULL;
    SDL_Texture *visual_selected = NULL;
    const char *filename = NULL;
    const char *selected_filename = NULL;
};

