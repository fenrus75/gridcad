#pragma once

#include "gridcad.h"
#include "value.h"
#include "port.h"
#include "name.h"

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
    
    bool in_edit_mode(void) override { return (name_edit->get_edit_mode() && single && selected) || (value_edit->get_edit_mode());};
    void enter_edit_mode(void) override { name_edit->set_edit_mode(true); selected = true; single = true;};    
    void rotate_ports(void) override;
    int get_width(void) { return ports[0]->get_width();};
    void set_width(int width) { ports[0]->set_width(width); if (width == 1) value.type = VALUE_TYPE_BOOL; else value.type = VALUE_TYPE_INT;};
    std::string get_verilog_main(void) override;
    std::string get_verilog_width(void) override;
private:
    std::string strvalue = "";
    struct value value = {};
    class name *name_edit = NULL;
    class name *value_edit = NULL;
};


