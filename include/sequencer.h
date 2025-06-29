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
	void enter_edit_mode(void) override { name_edit->set_edit_mode(true); selected = true; single = true;};
	void drawAt(class basecanvas *canvas, float X, float Y, int type) override;
	bool mouse_select(float X, float Y) override;
	void calculate(int ttl) override;
	void queued_calculate(int ttl) override;
	void reset_pointer(void) override { current_value = 0; queue_calculate(this); };
	void toggle_stop_error(void) { stop_clock_on_error = !stop_clock_on_error; };


protected:
	void add_value_row(void);
	void remove_value_row(void);
	void check_input(void);

	std::vector<struct value> values;
	unsigned int current_value = 0;
	bool current_clock = false;
	class name *name_edit = NULL;
	time_t previous_click = 0;
	class seqcanvas *canvas = NULL;
	bool is_error = false;
	bool stop_clock_on_error = false;
	
	friend class seqcanvas;
};
