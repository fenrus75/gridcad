#pragma once

#include "gridcad.h"
#include <cstdlib>
#include "dialog.h"


class synth : public dialog
{
public:
    synth(int screenX, int screenY, std::string projectname, std::string maketarget);
    virtual ~synth(void);
    void handle_event(class basecanvas *canvas, SDL_Event &event) override;
    
protected:
    FILE *pipe = NULL;
    std::string log2string(char *str);
    std::string frequency_line = "";
};