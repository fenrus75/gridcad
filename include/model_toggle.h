#pragma once

#include "gridcad.h"
#include "value.h"


class model_toggle : public element
{
public:
    model_toggle(float X, float Y);
    virtual ~model_toggle(void);
    virtual std::string class_id(void) { return "model_toggle:";};


    void drawAt(class canvas *canvas, float X, float Y, int type) override;
    bool mouse_select(float X, float Y) override;
    virtual void to_json(json &j) override;
    virtual void from_json(json &j) override;
private:
    struct value value = {};
    SDL_Texture *visual_off = NULL;
    SDL_Texture *visual_on = NULL;
    SDL_Texture *visual_selected = NULL;
};


