#pragma once
#include "gridcad.h"
#include "element.h"

class connector : public element
{
public:
    connector(float _X = 0, float _Y =0);
    virtual ~connector();

    virtual std::string class_id(void) { return "connector:";};
//    void draw(class canvas *canvas, int type) override;
    void drawAt(class basecanvas *canvas, float X, float Y, int type) override;

    void fill_grid(class wiregrid* grid) override;
    bool want_deleted(void) override;
    void draw_phase2(class basecanvas *canvas, int type) override;
    std::string get_verilog_main(void) override;

    void unsplice(void);
private:
};
