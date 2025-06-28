#include "gridcad.h"
#include "seqcanvas.h"
#include "sequencer.h"

seqcanvas::seqcanvas(class sequencer *_element) : basecanvas()
{
	seq = _element;

	window = SDL_CreateWindow(("Sequencer / "+ _element->get_full_name()).c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_RESIZABLE);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

	windowID = SDL_GetWindowID(window);
	
	editors.reserve(4096);
	
	seq_to_data();

	unhide();
	draw();    
}

seqcanvas::~seqcanvas(void)
{
	while (editors.size() > 0)
		delete_row();
	unregister_canvas(this);
	SDL_DestroyRenderer(renderer);
	renderer = NULL;
	SDL_DestroyWindow(window);
}

void seqcanvas::draw(void)
{
    SDL_SetRenderDrawColor(renderer, R(COLOR_BACKGROUND_GRID),
			       G(COLOR_BACKGROUND_GRID),
			       B(COLOR_BACKGROUND_GRID),
			       Alpha(COLOR_BACKGROUND_GRID));
			       
    SDL_RenderClear(renderer);
    
    draw_image("assets/model_truth/minus.png", 3,0,1,1);
    draw_image("assets/model_truth/plus.png", 5,0,1,1);
    
    for (unsigned int i = 0; i < editors.size(); i++) {
    	if (i == seq->current_value)
	    	draw_image("assets/lightgray.png", 1, 1+i,3,1);
    	draw_text_left("Time " + std::to_string(i), 1, 1 + i, 3.5,1);
    	if (i == cursor)
	    	draw_image("assets/lightgray.png", 4, 1+i,5,1);
    	auto e = editors[i];
    	e->drawAt(this, 4, 1 + i, 5, 1);
    }
    
      
    SDL_RenderPresent(renderer);
}

bool seqcanvas::handle_event(SDL_Event &event)
{
	switch (event.type) {
		case SDL_KEYDOWN:
	         switch (event.key.keysym.sym) {
		 case SDLK_UP:
		 	if (cursor > 0)
				make_active(cursor - 1);
	               break;
		 case SDLK_DOWN:
		 	make_active(cursor + 1);
	               break;
	        }
	        break;
	case SDL_MOUSEBUTTONDOWN:
		float x, y;
		x = scr_to_X(event.motion.x);
		y = scr_to_Y(event.motion.y);
		if (x >= 3 && x <= 4 && y >= 0 && y <= 1)
			delete_row();
		if (x >= 5 && x <= 6 && y >= 0 && y <= 1)
			add_row();
			
		if (y >= 1)
			make_active(y - 1);
		break;
	case SDL_WINDOWEVENT:
		if (event.window.event == SDL_WINDOWEVENT_CLOSE) {  /* user clicked the 'x' */
			SDL_HideWindow(window); /* TODO: THis leaks like a sieve */
		}
		break;
	};
	
	if (cursor < editors.size())
		editors[cursor]->handle_event(event);
    
	data_to_seq();
	return false;
}

void seqcanvas::seq_to_data(void)
{
	data.resize(seq->values.size());
	while (editors.size() < seq->values.size()) {
		class name *edit;
		edit = new class name(&data[data.size() - 1]);
		editors.push_back(edit);
	}
	while (editors.size() > seq->values.size()) {
		class name *edit;
		edit = editors[editors.size() - 1];
		delete edit;
		editors.pop_back();
	}
	
	for (unsigned int i = 0; i < seq->values.size(); i++) {
		data[i] = std::to_string(seq->values[i].intval);
	}
	
		for (unsigned int i = 0; i < editors.size(); i++) {
			auto e = editors[i];
			e->update_string_pointer(&data[i]);
		}
}

void seqcanvas::data_to_seq(void)
{
	seq->values.resize(data.size());
	for (unsigned int i = 0; i < seq->values.size(); i++) {
		seq->values[i].valid = true;
		seq->values[i].intval = std::stoi(data[i]);
		seq->values[i].boolval = std::stoi(data[i]);
	}
}


void seqcanvas::add_row(void)
{
		data.push_back("0");
		printf("Adding one %lu \n", data.size());
		class name *edit;
		unsigned i = data.size() - 1;
		edit = new class name(&data[i]);
		editors.push_back(edit);
		printf("Added one %lu  %s\n", editors.size(), data[i].c_str());
		
		for (unsigned int i = 0; i < editors.size(); i++) {
			auto e = editors[i];
			e->update_string_pointer(&data[i]);
			e->want_numbers_only();
		}
}

void seqcanvas::delete_row(void)
{
		data.pop_back();
		class name *edit;
		edit = editors[editors.size() - 1];
		delete edit;
		editors.pop_back();
		for (unsigned int i = 0; i < editors.size(); i++) {
			auto e = editors[i];
			e->update_string_pointer(&data[i]);
		}
}


void seqcanvas::make_active(unsigned int i)
{
	if (i >= data.size())
		return;
	cursor = i;
	for (unsigned p = 0; p < editors.size(); p++) {
		auto e = editors[p];
		e->set_edit_mode(p == cursor);
	}
	
}
