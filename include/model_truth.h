#pragma once

#include "gridcad.h"


class model_truth : public element
{
public:
    model_truth(float X, float Y);
    virtual ~model_truth(void);
    virtual std::string class_id(void) { return "model_truth:";};


    void drawAt(class canvas *canvas, float X, float Y, int type) override;
    bool mouse_select(float X, float Y) override;
    virtual void to_json(json &j) override;
    virtual void from_json(json &j) override;
private:
    SDL_Texture *visual_lt = NULL;
    SDL_Texture *visual_mt = NULL;
    SDL_Texture *visual_rt = NULL;
    SDL_Texture *visual_lm = NULL;
    SDL_Texture *visual_mm = NULL;
    SDL_Texture *visual_rm = NULL;
    SDL_Texture *visual_lb= NULL;
    SDL_Texture *visual_mb= NULL;
    SDL_Texture *visual_rb= NULL;
    SDL_Texture *visual_text = NULL;
    SDL_Texture *visual_selected = NULL;
    class truthcanvas *canvas = NULL;
};
