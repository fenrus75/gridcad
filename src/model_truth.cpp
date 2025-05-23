#include "gridcad.h"
#include "models.h"

model_truth::model_truth(float _X, float _Y)  : element(1, 1, "")
{
    sizeX = 4;
    sizeY = 4;    
    X = floorf(_X);
    Y = floorf(_Y);
    
}

model_truth::~model_truth(void)
{
}

void model_truth::drawAt(class canvas *canvas, float X, float Y, int type)
{
	int x,y;
    if (!visual_lt) {
        visual_lt = canvas->load_image("assets/model_truth/truth_lt.png");    
        visual_mt = canvas->load_image("assets/model_truth/truth_mt.png");    
        visual_rt = canvas->load_image("assets/model_truth/truth_rt.png");    
        visual_lm = canvas->load_image("assets/model_truth/truth_lm.png");    
        visual_mm = canvas->load_image("assets/model_truth/truth_mm.png");    
        visual_rm = canvas->load_image("assets/model_truth/truth_rm.png");    
        visual_lb = canvas->load_image("assets/model_truth/truth_lb.png");    
        visual_mb = canvas->load_image("assets/model_truth/truth_mb.png");    
        visual_rb = canvas->load_image("assets/model_truth/truth_rb.png");    
        visual_text = canvas->load_image("assets/model_truth/truthtable_text.png");    
        visual_selected = canvas->load_image("assets/model_truth/truthtable_selected.png");    
    }

    if (selected) {
	    canvas->draw_image(visual_selected, X, Y, sizeX, sizeY, Alpha(type));
    } else {
	    canvas->draw_image(visual_lt, X, Y, 1, 1, Alpha(type));
	    canvas->draw_image(visual_rt, X + sizeX - 1, Y, 1, 1, Alpha(type));
	    canvas->draw_image(visual_lb, X, Y + sizeY - 1, 1, 1, Alpha(type));
	    canvas->draw_image(visual_rb, X + sizeX - 1, Y + sizeY - 1, 1, 1, Alpha(type));
	    for (y = 1; y < sizeY - 1; y++)
		    canvas->draw_image(visual_lm, X, Y + y, 1, 1, Alpha(type));
	    for (y = 1; y < sizeY - 1; y++)
		    canvas->draw_image(visual_rm, X + sizeX - 1, Y + y, 1, 1, Alpha(type));
	    for (x = 1; x < sizeX - 1; x++)
		    canvas->draw_image(visual_mt, X + x, Y, 1, 1, Alpha(type));
	    for (x = 1; x < sizeX - 1; x++)
		    canvas->draw_image(visual_mb, X + x, Y + sizeY - 1, 1, 1, Alpha(type));
	
	    for (y = 1; y < sizeY -1 ; y++) 
		for (x = 1 ; x < sizeX -1 ; x++) {
		    canvas->draw_image(visual_mm, X + x, Y + y, 1, 1, Alpha(type));		
	        }
    }

    canvas->draw_image(visual_text, X + 0.2, Y + 0.2, sizeX - 0.2 , sizeY - 0.2, Alpha(type));

    for (auto port : ports) {
        port->drawAt(canvas, X,Y, COLOR_WIRE_SOLID);
        port->draw_wires(canvas);
    }
}



bool model_truth::mouse_select(float _X, float _Y)
{
	return false;
}

void model_truth::to_json(json &j)
{
     element::to_json(j);
}
void model_truth::from_json(json &j)
{
     element::from_json(j);
}
