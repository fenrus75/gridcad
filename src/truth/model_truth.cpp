#include "gridcad.h"
#include "model_truth.h"
#include "port.h"
#include "truthcanvas.h"

model_truth::model_truth(float _X, float _Y):element(1, 1, "")
{
	unsigned int i;
	sizeX = 8;
	sizeY = 4;
	X = floorf(_X);
	Y = floorf(_Y);
	
	inputs = 2;
	outputs = 1;
	
	values.resize((1 << inputs)); /* row 0 is the title bar/names */
	for (i = 0; i < values.size(); i++) {
		values[i].resize(inputs + outputs);
		for (unsigned int j = 0; j < inputs; j ++) {
			if ( (i >> j) & 1)
				values[i][j] = '1';
			else
				values[i][j] = '0';
		}
	}
		
	
		
	names.resize(inputs + outputs);
	for (i = 0; i < inputs; i++) {
		char buf[128];
		sprintf(buf, "In%i", i);
		names[i] = buf;
	}
	for (i = 0; i < outputs; i++) {
		char buf[128];
		sprintf(buf, "Out%i", i);
		names[inputs + i] = buf;
	}
}

model_truth::~model_truth(void)
{
}

void model_truth::drawAt(class canvas * canvas, float X, float Y, int type)
{
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

		canvas->draw_image(visual_lm, X, Y + 1, 1, sizeY - 2, Alpha(type));
		canvas->draw_image(visual_rm, X + sizeX - 1, Y + 1, 1, sizeY - 2, Alpha(type));
		canvas->draw_image(visual_mt, X + 1, Y, sizeX - 2, 1, Alpha(type));
		canvas->draw_image(visual_mb, X + 1, Y + sizeY - 1, sizeX - 2, 1, Alpha(type));

		canvas->draw_image(visual_mm, X + 1, Y + 1, sizeX - 2, sizeY - 2, Alpha(type));
	}

	canvas->draw_image(visual_text, X + 0.2, Y + 0.2, sizeX - 0.4, sizeY - 0.4, Alpha(type), true);

	for (auto port:ports) {
		port->drawAt(canvas, X, Y, COLOR_WIRE_SOLID);
		port->draw_wires(canvas);
	}
}

bool model_truth::mouse_select(float _X, float _Y)
{
	if (!canvas) {
		printf("Spawning a new window\n");
		canvas = new class truthcanvas(this);
		register_new_canvas(canvas);	
	} else {
		canvas->unhide();
	}
	return false;
}

void model_truth::to_json(json & j)
{
	element::to_json(j);
}

void model_truth::from_json(json & j)
{
	element::from_json(j);
}
