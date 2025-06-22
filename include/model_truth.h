#pragma once

#include "gridcad.h"
#include "element.h"
#include "name.h"

class truthcanvas;

class model_truth : public element
{
public:
    model_truth(float X, float Y);
    virtual ~model_truth(void);
    virtual std::string class_id(void) { return "model_truth:";};


    void drawAt(class basecanvas *canvas, float X, float Y, int type) override;
    bool mouse_select(float X, float Y) override;
    virtual void to_json(json &j) override;
    virtual void from_json(json &j) override;
    
    std::vector<std::vector<char>> values;
    std::vector<std::string> names;
    bool flip_input_ports = false;
    bool flip_output_ports = false;
    int get_inputs(void) { return inputs;};
    int get_output(void) { return outputs;};
    void turn_from_X(unsigned int X, unsigned int Y);    
    void turn_to_X(unsigned int X, unsigned int Y);
    void add_output(void);
    void del_output(void);
    void add_input(void);
    void del_input(void);
    void calculate(int ttl) override;
    void names_to_ports(void);
    void handle_event(class basecanvas *canvas, SDL_Event &event) override;
    bool in_edit_mode(void) override { return name_edit->get_edit_mode() && single && selected;};    
    void enter_edit_mode(void) override { name_edit->set_edit_mode(true); selected = true; single = true;};
    std::string get_verilog_main(void) override;
    std::string get_verilog_modules(std::string path) override;
    std::string get_verilog_name(void) override;
private:
    class truthcanvas *canvas = NULL;
    class name *name_edit = NULL;
    

    unsigned int inputs = 2;
    unsigned int outputs = 1;
    long int previous_click = -1;
    float mouseX = -100, mouseY = -100;     /* no save */
    std::string verilog_module_name = "";
};
