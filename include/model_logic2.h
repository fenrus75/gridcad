#pragma once

#include "gridcad.h"
#include "element.h"
#include "port.h"

class model_logic2 : public element
{
public:
    model_logic2(float X, float Y, std::string filename);
    virtual std::string class_id(void) { return "model_logic2:";};
    
    void drawAt(class basecanvas *canvas, float X, float Y, int type) override;
    
    void calculate(int ttl) override;
    
    virtual std::string get_verilog_operand(void) { return "XXXX_NOT_IMPLEMENTED_XXXX";}
    
    std::string get_verilog_main(void) override;
    void set_bus_width(int width);
    int get_bus_width(void) const { return bus_width; }

protected:
    uint64_t bus_mask(void) const;
    uint64_t read_port_u64(unsigned int idx) const;
    int bus_width = 1;
    std::string filename = "";
    std::string selected_filename = "";
};

