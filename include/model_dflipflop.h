#pragma once

#include "gridcad.h"
#include "value.h"
#include "port.h"

class model_dflipflop : public element
{
public:
    model_dflipflop(float X, float Y);
    virtual ~model_dflipflop(void);
    virtual std::string class_id(void) { return "model_dflipflop:";};


    void drawAt(class canvas *canvas, float X, float Y, int type) override;
    virtual void to_json(json &j) override;
    virtual void from_json(json &j) override;
    void handle_event(class canvas *canvas, SDL_Event &event);

    bool in_edit_mode(void) override { return edit_mode && single && selected;};
    void enter_edit_mode(void) override { edit_mode = true; selected = true; single = true;};
        
    void rotate_ports(void) override;
    int get_width(void) { return ports[0]->get_width();};
    void calculate(int ttl) override;
    void queued_calculate(int ttl) override;
    
private:
    struct value value = {};
    struct value previous_clock = {};
    bool edit_mode = false;
};

