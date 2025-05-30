#pragma once

#include "gridcad.h"
#include "value.h"

class model_delayline : public element
{
public:
    model_delayline(float X, float Y);
    virtual ~model_delayline(void);
    virtual std::string class_id(void) { return "model_delayline:";};
    
    void drawAt(class canvas *canvas, float X, float Y, int type) override;
    
    void calculate(int ttl) override;
    void queued_calculate(int ttl) override;
    void rotate_ports(void) override;
protected:
    struct value value = {};
};

