#pragma once

#include "gridcad.h"
#include "canvas.h"
#include "widget.h"

class truthcanvas : public basecanvas
{
public:
    truthcanvas(class model_truth *element);
    virtual ~truthcanvas();
    void draw(void) override;
    bool handle_event(SDL_Event &event) override;
    void add_widget(class widget *widget);
    
    void deselect_all(void);
    void unhide(void);
    void fill_grid(void);
    void clear_widgets(void);
    
    void add_output(void);
    void del_output(void);
    void add_input(void);
    void del_input(void);
    
    bool is_canX(int x, int y) {return canX[y][x];};
    void clear_canX(void);
    void turn_from_X(unsigned int X, unsigned int Y);
    void turn_to_X(unsigned int X, unsigned int Y);
    void values_changed(void);
    
private:
    class model_truth *element;
    std::vector<class widget *> widgets;
    bool need_fill_grid = false;
    
    int selectX, selectY;
    std::vector<std::vector<bool>> canX;
    void do_canX(unsigned int row);
};


