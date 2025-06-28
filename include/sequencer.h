#pragma once

#include "gridcad.h"
#include "element.h"
#include "value.h"
#include "name.h"

class seqcanvas;

class sequencer : public element
{
public:
	sequencer(int X, int Y);
	~sequencer();
	std::string class_id(void) override { return "sequencer:";};
	
	void to_json(json &j) override;
	void from_json(json &j) override;
        void handle_event(class basecanvas *canvas, SDL_Event &event) override;
	bool in_edit_mode(void) override { return name_edit->get_edit_mode() && single && selected;};    
	void drawAt(class basecanvas *canvas, float X, float Y, int type) override;
	bool mouse_select(float X, float Y) override;



protected:
	void add_value_row(void);
	void remove_value_row(void);

	std::vector<struct value> values;
	unsigned int current_value;
	class name *name_edit = NULL;
	time_t previous_click = 0;
	class seqcanvas *canvas = NULL;
	
	friend class seqcanvas;
};
