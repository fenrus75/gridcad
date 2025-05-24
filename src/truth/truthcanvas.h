#pragma once

#include "gridcad.h"

#include "widget.h"

class truthcanvas : public basecanvas
{
public:
    truthcanvas(class model_truth *element);
    virtual ~truthcanvas();
    void draw(void) override;
    bool handleEvent(SDL_Event &event) override;
    void add_widget(class widget *widget);
    
    void deselect_all(void);
    void unhide(void);
    void fill_grid(void);
    void clear_widgets(void);
    
    void add_output(void);
    void del_output(void);
    
private:
    class model_truth *element;
    std::vector<class widget *> widgets;
    bool need_fill_grid = false;
    
    int selectX, selectY;
};


