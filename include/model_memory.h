#pragma once

#include "gridcad.h"
#include "value.h"
#include "port.h"
#include "name.h"

class model_memory : public element
{
public:
    model_memory(float X, float Y);
    virtual ~model_memory(void);
    virtual std::string class_id(void) { return "model_memory:";};


    void drawAt(class canvas *canvas, float X, float Y, int type) override;
    virtual void to_json(json &j) override;
    virtual void from_json(json &j) override;
    void handle_event(class canvas *canvas, SDL_Event &event) override;

    bool in_edit_mode(void) override { return name_edit->get_edit_mode() && selected && single;};
    void enter_edit_mode(void) override { name_edit->set_edit_mode(true); selected = true; single = true;};
        
//    int get_width(void) { return ports[0]->get_width();};
    void calculate(int ttl) override;
    void queued_calculate(int ttl) override;
    std::string get_verilog_main(void) override;
    std::string get_verilog_modules(std::string path) override;
    unsigned int highest_addr_bit(unsigned int size);
    void update_memory_size(unsigned int newsize);
    
private:
    std::vector<std::uint8_t> data;
    struct value previous_clock = {};
    unsigned int previous_address = 0;
    class name *name_edit = NULL;
    std::string verilog_module_name = "";
};

