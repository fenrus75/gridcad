#pragma once

/* Project startup dialog logic */

#include "gridcad.h"



class projcanvas : public basecanvas
{
public:
    projcanvas(bool library_mode = false);
    virtual ~projcanvas(void);
    

    void draw(void) override;
    bool handle_event(SDL_Event &event) override;
    
    
    void run(void);
    
    std::string get_name(void) {if (!library_mode) return name; return library_test_bench();};

protected:
    std::string name = "default";
    std::vector<std::string> templates;
    std::vector<std::string> projects;
    
    std::map<std::string, std::string> template_descriptions;
    unsigned int active_template = 0;
    unsigned int active_project = 0;
    
    unsigned int hover_project = 0;
    unsigned int hover_template = 0;
    bool hover_left_button = false;
    bool hover_right_button = false;
    uint64_t hover_timestamp = 0;
    
    float currentX = 0, currentY = 0;
    
    class name *newname = NULL;
    
    void crawl_filesystem(void);
    void crawl_library(void);
    void create_project_from_template(std::string name, std::string templ);
    std::string library_test_bench(void);
    time_t previous_click = 0;
    bool library_mode = false;
};