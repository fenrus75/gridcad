#pragma once

#include "gridcad.h"
#include "value.h"
#include "port.h"
#include "name.h"

class model_output : public element
{
public:
    model_output(float X, float Y);
    virtual ~model_output(void);
    virtual std::string class_id(void) { return "model_output:";};


    void drawAt(class basecanvas *canvas, float X, float Y, int type) override;
    virtual void to_json(json &j) override;
    virtual void from_json(json &j) override;
    void handle_event(class basecanvas *canvas, SDL_Event &event) override;
    struct value get_value(void);
    
    bool in_edit_mode(void) override { return name_edit->get_edit_mode() && single && selected;};
    void enter_edit_mode(void) override { name_edit->set_edit_mode(true); selected = true; single = true;};    
    void rotate_ports(void) override;
    void set_width(int width) { ports[0]->set_width(width); if (width == 1) value.type = VALUE_TYPE_BOOL; else value.type = VALUE_TYPE_INT;};
    int get_width(void) { return ports[0]->get_width();};
    std::string get_verilog_main(void) override;
    std::string get_verilog_width(void) override;
    void create_verilog_name(int seqno, std::vector<std::string> *existing) override;
private:
    struct value value = {};
    class name *name_edit = NULL;
};

