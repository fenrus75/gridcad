#pragma once

#include "gridcad.h"
#include "value.h"
#include "port.h"

class model_clock : public element
{
public:
    model_clock(float X, float Y);
    virtual ~model_clock(void);
    virtual std::string class_id(void) { return "model_clock:";};


    void drawAt(class canvas *canvas, float X, float Y, int type) override;
    virtual void to_json(json &j) override;
    virtual void from_json(json &j) override;
    struct value get_value(void);
    void handle_event(class canvas *canvas, SDL_Event &event);
    void rotate_ports(void);
    
private:
    struct value value = {};
};

