#pragma once

#include "gridcad.h"
#include "element.h"
#include "port.h"

class model_logic2 : public element
{
public:
    model_logic2(float X, float Y, std::string filename);
    virtual ~model_logic2(void);
    virtual std::string class_id(void) { return "model_logic2:";};
    
    void drawAt(class basecanvas *canvas, float X, float Y, int type) override;
    
    void calculate(int ttl) override;
    
    virtual std::string get_verilog_operand(void) { return "XXXX_NOT_IMPLEMENTED_XXXX";}
    
    std::string get_verilog_main(void) override;

protected:
    std::string filename = "";
    std::string selected_filename = "";
};

