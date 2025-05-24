#pragma once

#include "gridcad.h"


class model_truth : public element
{
public:
    model_truth(float X, float Y);
    virtual ~model_truth(void);
    virtual std::string class_id(void) { return "model_truth:";};


    void drawAt(class canvas *canvas, float X, float Y, int type) override;
    bool mouse_select(float X, float Y) override;
    virtual void to_json(json &j) override;
    virtual void from_json(json &j) override;
    
    std::vector<std::vector<char>> values;
    std::vector<std::string> names;
    int get_inputs(void) { return inputs;};
    int get_output(void) { return outputs;};
    
    void add_output(void);
    void del_output(void);
    
private:
    class truthcanvas *canvas = NULL;
    
    unsigned int inputs = 2;
    unsigned int outputs = 1;
    
};
