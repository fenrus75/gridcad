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
				values[i][inputs - j - 1] = '1';
			else
				values[i][inputs - j - 1] = '0';
		}
		for (unsigned int j = 0; j < outputs; j ++) {
			values[i][inputs + j] = 'X';
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
	if (selected) {
		canvas->draw_image("assets/model_truth/truthtable_selected.png", X, Y, sizeX, sizeY, Alpha(type));
	} else {
		canvas->draw_image("assets/model_truth/truth_lt.png", X, Y, 1, 1, Alpha(type));
		canvas->draw_image("assets/model_truth/truth_rt.png", X + sizeX - 1, Y, 1, 1, Alpha(type));
		canvas->draw_image("assets/model_truth/truth_lb.png", X, Y + sizeY - 1, 1, 1, Alpha(type));
		canvas->draw_image("assets/model_truth/truth_rb.png", X + sizeX - 1, Y + sizeY - 1, 1, 1, Alpha(type));

		canvas->draw_image("assets/model_truth/truth_lm.png", X, Y + 1, 1, sizeY - 2, Alpha(type));
		canvas->draw_image("assets/model_truth/truth_rm.png", X + sizeX - 1, Y + 1, 1, sizeY - 2, Alpha(type));
		canvas->draw_image("assets/model_truth/truth_mt.png", X + 1, Y, sizeX - 2, 1, Alpha(type));
		canvas->draw_image("assets/model_truth/truth_mb.png", X + 1, Y + sizeY - 1, sizeX - 2, 1, Alpha(type));

		canvas->draw_image("assets/model_truth/truth_mm.png", X + 1, Y + 1, sizeX - 2, sizeY - 2, Alpha(type));
	}

	canvas->draw_image("assets/model_truth/truthtable_text.png", X + 0.2, Y + 0.2, sizeX - 0.4, sizeY - 0.4, Alpha(type), true);

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
	j["names"] = names;
	j["values"] = values;
	j["inputs"] = inputs;
	j["outputs"] = outputs;
}

void model_truth::from_json(json & j)
{
	element::from_json(j);

	names = j["names"];
	values = j["values"];	
	inputs = j["inputs"];
	outputs = j["outputs"];
		
}

void model_truth::add_output(void)
{
	unsigned int y;
	char buf[128];
	
	for (y = 0; y < values.size(); y++)
		values[y].push_back('X');
	outputs++;
	sprintf(buf, "Out%i", outputs -1);
	names.push_back(buf);
	
	/* TODO -- spwan a port as well */
}

void model_truth::del_output(void)
{
	unsigned int y;
	
	if (outputs <= 1)
		return;
	
	for (y = 0; y < values.size(); y++)
		values[y].pop_back();
	outputs--;
	names.pop_back();
	
	/* TODO -- spwan a port as well */
}

static bool compare_line(std::vector<char> A, std::vector<char> B)
{
	unsigned int x;
	for (x = 0; x < A.size(); x++) {
		if (A[x] < B[x])
			return true;
		if (A[x] > B[x])
			return false;
	}
	
	return false;
}

static bool same_inputs(std::vector<char> A, std::vector<char> B, unsigned int inputs)
{
	unsigned int x;
	for (x = 0; x < inputs; x++) {
		if (A[x] != B[x])
			return false;
	}
	
	return true;
}


void model_truth::add_input(void)
{
	unsigned int y;
	char buf[128];
	
	printf("Adding input\n");

	std::vector<std::vector<char>> values2;	
	
	values2 = values;
	
	sprintf(buf, "In%i", inputs);
	names.insert(names.begin() + inputs , buf);
	for (y = 0; y < values.size(); y++) {
		values[y].insert(values[y].begin() + inputs , '0');
		values2[y].insert(values2[y].begin() + inputs , '1');
	}
	values.insert(values.end(), values2.begin(), values2.end());
	inputs++;
	
	std::sort(values.begin(), values.end(), compare_line);
	
	/* TODO -- spwan a port as well */
}

void model_truth::del_input(void)
{
	unsigned int y;
	char buf[128];
	
	sprintf(buf, "In%i", inputs);
	names.erase(names.begin() + inputs-1);
	for (y = 0; y < values.size(); y++) {
		values[y].erase(values[y].begin() + inputs -1);
	}
	inputs--;

	/* we go backwards to avoid O(N^2) operations */	
	for (y = values.size() -1; y > 0; y--) {
		if (same_inputs(values[y], values[y-1], inputs)) {
			values.erase(values.begin() + y);
		}
	}
	
	
	/* TODO -- spwan a port as well */
}

