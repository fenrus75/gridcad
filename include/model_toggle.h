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
    void to_json(json &j) override;
    void from_json(json &j) override;
    void handle_event(SDL_Event &event);
    
    bool in_edit_mode(void) { return edit_mode && single && selected;};
private:
    struct value value = {};
    bool edit_mode = false;
};


