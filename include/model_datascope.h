#pragma once

#include "gridcad.h"
#include "value.h"
#include "port.h"

class model_datascope : public element
{
public:
    model_datascope(float X, float Y);
    std::string class_id(void) override { return "model_datascope:";};


    void drawAt(class basecanvas *canvas, float X, float Y, int type) override;
    void to_json(json &j) override;
    void from_json(json &j) override;

    bool in_edit_mode(void) override { return edit_mode && single && selected;};
    
    int get_width(void) { return ports[0]->get_width();};
    std::string get_verilog_main(void) override {return "";};
    void rotate_ports(void) override;
    void set_format(int f) { format = f; };
    int get_format(void) const { return format; };
    void set_scale_2x(bool enabled);
    void toggle_scale_2x(void) { set_scale_2x(!scale_2x); };
    bool is_scale_2x(void) const { return scale_2x; };
    std::string format_display_text(void) const;
    std::string connected_wire_label(void) const;
    class wire *connected_vcd_wire(void) const;
private:
    struct value value = {};
    bool edit_mode = false;
    bool scale_2x = false;
    /* 0 = decimal, 1 = hex, 2 = binary, 3 = enum (falls back to decimal if no label) */
    int format = 3;
};

