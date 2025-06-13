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
    void handle_event(class canvas *canvas, SDL_Event &event) override;
    void rotate_ports(void) override;
    class port *get_clk_port(void) override { return ports[0];};
    std::string get_verilog_main(void) override;
    bool mouse_select(float _X, float _Y) override;
private:
};



extern struct value global_clock;
extern bool clock_running;