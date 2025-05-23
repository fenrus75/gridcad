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

	void run(void);

private:
	std::string name;
	
};

