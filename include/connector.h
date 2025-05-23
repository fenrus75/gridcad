#pragma once
#include "gridcad.h"

class connector : public element
{
public:
    connector(float _X = 0, float _Y =0);
    virtual ~connector();

    virtual std::string class_id(void) { return "connector:";};
    void draw(class canvas *canvas, int type) override;
    void fill_grid(class wiregrid* grid) override;
    bool want_deleted(void) override;
private:
    SDL_Texture *red = NULL;
    SDL_Texture *green = NULL;
    SDL_Texture *open = NULL;
};
