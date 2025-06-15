#include "gridcad.h"
#include "dialog.h"
#include "contextmenu.h"

dialog::dialog(int _screenX, int _screenY, std::string text)
{
    screenX = _screenX;
    screenY = _screenY;
    minwtext = text;
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
    maxX = std::max(maxX, Xsize(canvas, minwtext));
    maxY = std::max(maxY, Ysize(canvas, minwtext));


    /* remember the maximum width over time so that the dialog box doesn't get jumpy in width as things scroll through */    
    if (maxX > largestX) {
        largestX = maxX;
    } else {
        maxX = largestX;
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
    
    canvas->draw_image("assets/dialog.png", posX - canvas->scale_to_X(5), posY - canvas->scale_to_Y(5), maxX + canvas->scale_to_X(10), maxY * lines + canvas->scale_to_Y(10));
    
    float cX, cY;
    
    cX = posX + canvas->scale_to_X(5);
    cY = posY + canvas->scale_to_Y(5);
    
    for (auto line : content) {
        canvas->draw_text_left(line, cX, cY, maxX, maxY);
        cY += maxY;
    }

    /* TODO: fix the mess below by just storing a few things in variables and reuse */    
    if (has_ok_button) {
        float X1, Y1, W, H;
        float marginX, marginY;
        cY += maxY;


        X1 = posX + (maxX - canvas->scale_to_X(Xsize(canvas, "  OK  ")/get_scale()))/2;
        Y1 = cY;
        W = canvas->scale_to_X(Xsize(canvas, "  OK  ")/get_scale() );
        H = maxY;
        
        buttonX1 = X1;
        buttonY1 = Y1;
        buttonW = W;
        buttonH = H;
        
        marginX = canvas->scale_to_X(10);
        marginY = canvas->scale_to_Y(10);
        
        if (currentX >= X1 && currentX <= X1 + W && currentY >= Y1 && currentY <= Y1 + H)
            canvas->draw_image("assets/buttonbox_hover.png", X1 - marginX/2, Y1 - marginY/2, W + marginX, maxY + marginY); 
        else
            canvas->draw_image("assets/buttonbox.png", X1 - marginX/2, Y1 - marginY/2, W + marginX, maxY + marginY); 
                
        canvas->draw_text("OK", X1, Y1, W, H);
    }

}

void dialog::handle_event(class canvas *canvas, SDL_Event &event)
{
	switch (event.type) {
		case SDL_MOUSEBUTTONDOWN:
			float x, y;
                        x = canvas->scr_to_X(event.motion.x);
                        y = canvas->scr_to_Y(event.motion.y);

                        if (has_ok_button && x >= buttonX1 && x <= buttonX1 + buttonW && y >= buttonY1 && y <= buttonY1 + buttonH) {
                        	SDL_Event ev = {};

                                ev.type = EVENT_CLOSE_DIALOG;
                        	ev.user.code = 0;
                        	ev.user.data1 = canvas->get_scene();
	
                        	SDL_PushEvent(&ev);                            
                        }

		case SDL_MOUSEMOTION:
                        currentX = canvas->scr_to_X(event.motion.x);
                        currentY = canvas->scr_to_Y(event.motion.y);
                        break;
	}	
}
