#pragma once

#include "gridcad.h"
#include "port.h"

class model_logic2 : public element
{
public:
    model_logic2(float X, float Y, std::string filename);
    virtual ~model_logic2(void);
    virtual std::string class_id(void) { return "model_logic2:";};
    
    void drawAt(class canvas *canvas, float X, float Y, int type) override;
    
    void calculate(int ttl) override;
    void rotate_ports(void) override;

protected:
    std::string filename = "";
    std::string selected_filename = "";
};

