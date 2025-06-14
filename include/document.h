#pragma once
#include "gridcad.h"

/* Main class -- handles events etc.
   A document has scenes (represented via canvases) which have elements/etc
 */
class document
{
public:
	document(std::string name);
	~document(void);
	
	void save_verilog(std::string path, std::string filename);
	void save_json(std::string filename);

	void run(void);

private:
	std::string name = "";
	
};

