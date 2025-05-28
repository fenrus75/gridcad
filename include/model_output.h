#pragma once

#include "gridcad.h"
#include "value.h"
#include "port.h"

class model_output : public element
{
public:
    model_output(float X, float Y);
    virtual ~model_output(void);
    virtual std::string class_id(void) { return "model_output:";};


    void drawAt(class canvas *canvas, float X, float Y, int type) override;
    virtual void to_json(json &j) override;
    virtual void from_json(json &j) override;
    void handle_event(class canvas *canvas, SDL_Event &event);
    struct value get_value(void);
    
    bool in_edit_mode(void) override { return edit_mode && single && selected;};
    
    void rotate_ports(void) override;
    int get_width(void) { return ports[0]->get_width();};
private:
    struct value value = {};
    bool edit_mode = false;
};

