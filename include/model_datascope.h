#pragma once

#include "gridcad.h"
#include "value.h"
#include "port.h"

class model_datascope : public element
{
public:
    model_datascope(float X, float Y);
    virtual ~model_datascope(void);
    virtual std::string class_id(void) { return "model_datascope:";};


    void drawAt(class canvas *canvas, float X, float Y, int type) override;
    virtual void to_json(json &j) override;
    virtual void from_json(json &j) override;

    bool in_edit_mode(void) override { return edit_mode && single && selected;};
    
    int get_width(void) { return ports[0]->get_width();};
    void rotate_ports(void) override;
private:
    struct value value = {};
    bool edit_mode = false;
};

