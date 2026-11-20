#pragma once
#include "gridcad.h"
class vcd_session;

/* Main class -- handles events etc.
   A document has scenes (represented via canvases) which have elements/etc
 */
class document
{
public:
	document(std::string name, bool library_mode = false,
	         std::string import_json_path = "",
	         std::string vcd_path = "",
	         std::string vcd_scope = "");
	~document(void);
	
	void save_verilog(std::string_view path, std::string_view filename);
	void save_json(void);

	void run(void);

private:
	std::string name = "";
	struct timeval previous_slow_draw = {};  /* allows for throttled framerate draws of non-focus windows*/
	struct timeval previous_fast_draw = {};  /* foreground/max framerate */
	unsigned int fast_draw_counter = 0;
	
	bool library_mode = false;
	bool import_mode = false;
	std::unique_ptr<class canvas> main_canvas;
	std::unique_ptr<class vcd_session> vcd;
	sdl::Timer clock_timer;
	
};