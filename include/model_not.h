#pragma once

#include "gridcad.h"


class model_not : public element
{
public:
    model_not(float X, float Y);
    virtual std::string class_id(void) { return "model_not:";};
    
    void drawAt(class basecanvas *canvas, float X, float Y, int type) override;
    
    void calculate(int ttl) override;
    void rotate_ports(void) override;
    std::string get_verilog_main(void) override;
    void set_bus_width(int width);
    int get_bus_width(void) const { return bus_width; }
    void to_json(json &j) override;
    void from_json(json &j) override;
protected:
    int bus_width = 1;
};

