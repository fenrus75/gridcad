#pragma once

#include "gridcad.h"
#include "value.h"
#include "port.h"

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
    void handle_event(class canvas *canvas, SDL_Event &event);
    void update_value(struct value *value, int ttl);
    
    bool in_edit_mode(void) override { return edit_mode && single && selected;};
    void rotate_ports(void) override;
    int get_width(void) { return ports[0]->get_width();};
private:
    struct value value = {};
    bool edit_mode = false;
};


