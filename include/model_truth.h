#pragma once

#include "gridcad.h"
#include "element.h"
#include "name.h"

class truthcanvas;

class model_truth : public element
{
public:
    model_truth(float X, float Y);
    ~model_truth(void) override;
    std::string class_id(void) override { return "model_truth:";};


    void drawAt(class basecanvas *canvas, float X, float Y, int type) override;
    bool mouse_select(float X, float Y) override;
    void to_json(json &j) override;
    void from_json(json &j) override;
    
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
    /* Configure as a 1- or multi-output LUT (num_inputs in [1, 8], MSB-first bitstrings). */
    void configure_lut(int num_inputs, std::string_view lut_bits);
    void configure_lut(int num_inputs, const std::vector<std::string> &lut_outputs);
    /* Configure as a vector/mixed-width LUT whose input port widths sum to total_bits in [1, 8]. */
    void configure_lut_packed(const std::vector<int> &in_widths,
                              const std::vector<std::string> &in_names,
                              const std::vector<std::string> &lut_outputs);
    void calculate(int ttl) override;
    void names_to_ports(void);
    void handle_event(class basecanvas *canvas, SDL_Event &event) override;
    bool in_edit_mode(void) override { return name_edit->get_edit_mode() && single && selected;};    
    void enter_edit_mode(void) override { name_edit->set_edit_mode(true); selected = true; single = true;};
    std::string get_verilog_main(void) override;
    std::string get_verilog_modules(std::string path) override;
    std::string get_verilog_name(void) override;
private:
    std::unique_ptr<class truthcanvas> canvas;
    std::unique_ptr<class name> name_edit;
    

    unsigned int inputs = 2;
    unsigned int outputs = 1;
    long int previous_click = -1;
    float mouseX = -100, mouseY = -100;     /* no save */
    std::string verilog_module_name = "";
    std::vector<int> input_port_widths;     /* non-empty when configured with vector/mixed-width LUT inputs */
    std::vector<std::string> packed_lut_outputs;
};
