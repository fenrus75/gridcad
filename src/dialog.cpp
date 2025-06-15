#include "gridcad.h"
#include "dialog.h"
#include "contextmenu.h"

dialog::dialog(int _screenX, int _screenY)
{
    screenX = _screenX;
    screenY = _screenY;
}

dialog::~dialog(void)
{
}

void dialog::draw(class basecanvas *canvas)
{
    float maxX = 0;
    float maxY = 0;
    int lines = content.size();
    
    if (has_ok_button) 
        lines += 3;
    
    float posX, posY;
   /* step 1: how large is the text */
    maxX = 0;
    maxY = 0;

    for (auto item : content) {
        maxX = std::max(maxX, Xsize(canvas, item));
        maxY = std::max(maxY, Ysize(canvas, item));
    }
    
    maxX = maxX / get_scale();
    maxY = maxY / get_scale();
    
    /* center the dialog box */
    posX = (screenX - maxX - 10)/2;
    posY = (screenY - maxY * lines - 10)/2;
    /* upto here we're in screen pixels */
    
    maxX = canvas->scale_to_X(maxX);
    maxY = canvas->scale_to_Y(maxY);
    posX = canvas->scr_to_X(posX);
    posY = canvas->scr_to_Y(posY);
    
    canvas->draw_image("assets/lightgray.png", canvas->scr_to_X(0), canvas->scr_to_Y(0), canvas->scale_to_X(screenX), canvas->scale_to_Y(screenY));
    
    canvas->draw_image("assets/dialog.png", posX, posY, maxX + canvas->scale_to_X(10), maxY * lines + canvas->scale_to_Y(10));
    
    float cX, cY;
    
    cX = posX + canvas->scale_to_X(5);
    cY = posY + canvas->scale_to_Y(5);
    
    for (auto line : content) {
        canvas->draw_text_left(line, cX, cY, maxX, maxY);
        cY += maxY;
    }

    /* TODO: fix the mess below by just storing a few things in variables and reuse */    
    if (has_ok_button) {
        cY += maxY;
        canvas->draw_image("assets/buttonbox.png", posX + (maxX - canvas->scale_to_X(Xsize(canvas, "  OK  ")/get_scale() - 5))/2, cY - canvas->scale_to_Y(5), canvas->scale_to_X(Xsize(canvas, "  OK  ")/get_scale() + 10), maxY + canvas->scale_to_Y(10)); 
                
        canvas->draw_text("OK", posX + (maxX - canvas->scale_to_X(Xsize(canvas, "  OK  ")/get_scale() - 0))/2, cY - canvas->scale_to_Y(0), canvas->scale_to_X(Xsize(canvas, "  OK  ")/get_scale() + 0), maxY + canvas->scale_to_Y(0)); 
    }

}

void dialog::handle_event(SDL_Event &event)
{
}

