#pragma once

#include "gridcad.h"



class hexcanvas : public basecanvas
{
public :
    hexcanvas(std::string name, unsigned char *buffer, unsigned int length);
    ~hexcanvas(void);
    
    bool handle_event(SDL_Event &event) override;
    void draw(void) override;
    
private:
    unsigned char *buffer = NULL;
    unsigned int length = 0;
};