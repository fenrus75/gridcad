#pragma once

#include "gridcad.h"

class sequencer;

class seqcanvas : public basecanvas
{
public:
	seqcanvas(class sequencer *seq);
	~seqcanvas(void) override;
	void draw(void) override;
	bool handle_event(SDL_Event &event) override;
protected:
	class sequencer *seq;
	
	void data_to_seq(void);
	void seq_to_data(void);
	void add_row(void);
	void delete_row(void);
	void make_active(unsigned int i);
	
	std::vector<std::string> data;
	std::vector<std::unique_ptr<class name>> editors;
	unsigned int cursor = 0;
};
