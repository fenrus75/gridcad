#pragma once
#include "gridcad.h"

/* Main class -- handles events etc.
   A document has scenes (represented via canvases) which have elements/etc
 */
class document
{
public:
	document(std::string name, bool library_mode = false);
	~document(void);
	
	void save_verilog(std::string path, std::string filename);
	void save_json(void);

	void run(void);

private:
	std::string name = "";
	struct timeval previous_slow_draw = {};  /* allows for throttled framerate draws of non-focus windows*/
	struct timeval previous_fast_draw = {};  /* foreground/max framerate */
	unsigned int fast_draw_counter = 0;
	
	bool library_mode = false;
	
};

extern void init_SDL(void);