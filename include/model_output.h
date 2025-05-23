#pragma once

#include "gridcad.h"


class model_output : public element
{
public:
    model_output(float X, float Y);
    virtual ~model_output(void);
    virtual std::string class_id(void) { return "model_output:";};


    void drawAt(class canvas *canvas, float X, float Y, int type) override;
    virtual void to_json(json &j) override;
    virtual void from_json(json &j) override;
private:
    struct value value = {};
    SDL_Texture *visual_off = NULL;
    SDL_Texture *visual_on = NULL;
    SDL_Texture *visual_selected = NULL;
};

