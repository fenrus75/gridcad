#include "gridcad.h"
#include "project.h"
#include "name.h"
#include "document.h"

#include <fstream>
#include <string>
#include <sstream>

extern void fill_png_maps(void);



projcanvas::projcanvas(bool _library_mode)
{
        std::string s;
        library_mode = _library_mode;
	s = "GridCad Welcome Screen";;
	
	init_SDL();
	fill_png_maps();
	
	window = SDL_CreateWindow(s.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1024, 768, SDL_WINDOW_RESIZABLE);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	
	scaleX = 25;
	scaleY = 25;

	newname	= new class name(&name);
	newname->set_edit_mode(true);
	
	templates.push_back("<None>");
	template_descriptions["<None>"] = "Blank Canvas";

	if (library_mode)
		crawl_library();
	else
		crawl_filesystem();
}

projcanvas::~projcanvas(void)
{
        delete newname;
        SDL_DestroyRenderer(renderer);
        renderer = NULL;
        SDL_DestroyWindow(window);
}

#define RADIO_HEIGHT 1.5
#define TEMPLATE_X 2
#define TEMPLATE_Y 7.5
#define TEMPLATE_WIDTH 18
#define PROJECT_X (TEMPLATE_X + TEMPLATE_WIDTH + 2)
#define PROJECT_Y 4.5
#define PROJECT_WIDTH 18
#define PER_COLUMN 16
static float calc_project_X(int index)
{
	int column = index / PER_COLUMN;
	return PROJECT_X + (column * (PROJECT_WIDTH + 1));
}

static float calc_project_Y(int index)
{
	return PROJECT_Y + 0.2 + RADIO_HEIGHT * (index % PER_COLUMN);
}

static float calc_project_height(int index)
{
	if (index < PER_COLUMN)
		return index *  RADIO_HEIGHT;
	else
		return PER_COLUMN * RADIO_HEIGHT;
}

void projcanvas::draw(void)
{
	SDL_SetRenderDrawColor(renderer, R(COLOR_BACKGROUND_GRID),
			       G(COLOR_BACKGROUND_GRID),
			       B(COLOR_BACKGROUND_GRID),
			       Alpha(COLOR_BACKGROUND_GRID));
	SDL_RenderClear(renderer);

	float X = 0, Y = 0;
	while (Y < 100) {
		X = 0;
		while (X < 100) {
			draw_box(X + 0.15, Y + 0.15, X + 9.85, Y + 9.85,
				COLOR_BACKGROUND_MAIN);
			X = X + 10;
		}
		Y = Y + 10;
	}
	
	draw_image("assets/label.png", TEMPLATE_X - 0.5, 2.5, TEMPLATE_WIDTH + 1, RADIO_HEIGHT * templates.size() + 9.5);

	draw_text_left("New project name:", TEMPLATE_X, 3,15,1);
	draw_image("assets/radiobox.png", TEMPLATE_X - 0.2 , 4, 15, 1.4);
	newname->drawAt(this, TEMPLATE_X, 4.25, 15, 1);
	draw_text_left("Project template to use", TEMPLATE_X, 6, 15, 1);
	
	for (unsigned int T = 0; T < templates.size(); T++) {
		if (T == active_template)
			draw_image("assets/radiobox_selected.png", TEMPLATE_X - 0.2, TEMPLATE_Y + RADIO_HEIGHT * T, TEMPLATE_WIDTH, RADIO_HEIGHT - 0.1);
		else {
			if (currentX >= TEMPLATE_X && currentX <= TEMPLATE_X + TEMPLATE_WIDTH && currentY > TEMPLATE_Y + RADIO_HEIGHT * T && currentY <= TEMPLATE_Y + RADIO_HEIGHT * (T+1))
				draw_image("assets/radiobox_hover.png", TEMPLATE_X - 0.2, TEMPLATE_Y + RADIO_HEIGHT * T, TEMPLATE_WIDTH , RADIO_HEIGHT - 0.1);
			else
				draw_image("assets/radiobox.png", TEMPLATE_X - 0.2, TEMPLATE_Y + RADIO_HEIGHT * T, TEMPLATE_WIDTH , RADIO_HEIGHT - 0.1);
		}
			
		draw_text_left(template_descriptions[templates[T]], TEMPLATE_X, TEMPLATE_Y + 0.2 + RADIO_HEIGHT * T, TEMPLATE_WIDTH -1, 1);
	}
	
	
	if (currentX >= TEMPLATE_X + 4 && currentX <= TEMPLATE_X + TEMPLATE_WIDTH && currentY >= TEMPLATE_Y + RADIO_HEIGHT * (templates.size() + 1) && currentY <= TEMPLATE_Y + RADIO_HEIGHT * (templates.size() + 2))
		draw_image("assets/buttonbox_hover.png", TEMPLATE_X + 4, TEMPLATE_Y + RADIO_HEIGHT * (templates.size() + 1), TEMPLATE_WIDTH - 4, RADIO_HEIGHT - 0.1);	
	else
		draw_image("assets/buttonbox.png", TEMPLATE_X + 4, TEMPLATE_Y + RADIO_HEIGHT * (templates.size() + 1), TEMPLATE_WIDTH - 4, RADIO_HEIGHT - 0.1);	
	
	bool is_existing = false;
	for (auto e : projects) {
		if (e == name)
			is_existing = true;
	}
	
	if (is_existing)
		draw_text("Open project " + name, TEMPLATE_X + 4, TEMPLATE_Y + RADIO_HEIGHT * (templates.size() + 1)+0.2, TEMPLATE_WIDTH - 4, 1);
	else
		draw_text("Create project " + name, TEMPLATE_X + 4, TEMPLATE_Y + RADIO_HEIGHT * (templates.size() + 1)+0.2, TEMPLATE_WIDTH - 4, 1);
	



	for (unsigned T = 0; T < (1 + projects.size()/PER_COLUMN) ; T++)
		draw_image("assets/label2.png", PROJECT_X - 0.5 + T * (PROJECT_WIDTH+1), 2.5, PROJECT_WIDTH + 1, calc_project_height(projects.size()) + 5.5);
	draw_text_left("Existing projects", PROJECT_X,3,15,1);
	for (unsigned int T = 0; T < projects.size(); T++) {
		if (T == active_project)
			draw_image("assets/radiobox_selected.png", calc_project_X(T) - 0.2, calc_project_Y(T) - 0.2, PROJECT_WIDTH, RADIO_HEIGHT - 0.1);
		else {
			if (currentX >= calc_project_X(T) && currentX <= calc_project_X(T) + PROJECT_WIDTH && currentY > calc_project_Y(T)-0.2 && currentY <= calc_project_Y(T) +  RADIO_HEIGHT)
				draw_image("assets/radiobox_hover.png", calc_project_X(T) - 0.2, calc_project_Y(T) - 0.2, PROJECT_WIDTH, RADIO_HEIGHT - 0.1);
			else
				draw_image("assets/radiobox.png", calc_project_X(T) - 0.2, calc_project_Y(T) - 0.2 , PROJECT_WIDTH, RADIO_HEIGHT - 0.1);
		}
			
		draw_text_left(projects[T], calc_project_X(T), calc_project_Y(T), PROJECT_WIDTH - 1, 1);
	}

	if (currentX >= PROJECT_X + 4 && currentX <= PROJECT_X + PROJECT_WIDTH && currentY >= PROJECT_Y + calc_project_height(projects.size()) + RADIO_HEIGHT && currentY <= PROJECT_Y + calc_project_height(projects.size()) + 2 * RADIO_HEIGHT)
		draw_image("assets/buttonbox_hover.png", PROJECT_X + 4, PROJECT_Y + calc_project_height(projects.size()) + RADIO_HEIGHT, PROJECT_WIDTH - 4.2, RADIO_HEIGHT - 0.1);	
	else
		draw_image("assets/buttonbox.png", PROJECT_X + 4, PROJECT_Y + calc_project_height(projects.size()) + RADIO_HEIGHT, PROJECT_WIDTH - 4.2, RADIO_HEIGHT - 0.1);	
	draw_text("Open project " + projects[active_project], PROJECT_X + 4, PROJECT_Y + calc_project_height(projects.size()) + RADIO_HEIGHT + 0.2, PROJECT_WIDTH - 4, 1);
	
	SDL_RenderPresent(renderer);
	
}

bool projcanvas::handle_event(SDL_Event &event)
{
	switch (event.type) {
                case SDL_QUIT:
                        printf("QUIT\n");
                        exit(0);
                        return true;
                        break;
		case SDL_MOUSEBUTTONDOWN:
			float x, y;
                        x = scr_to_X(event.motion.x);
                        y = scr_to_Y(event.motion.y);
                        
                        if (x >= TEMPLATE_X && x < TEMPLATE_X + TEMPLATE_WIDTH && y > TEMPLATE_Y) {
                        	unsigned int tY = floor((y - TEMPLATE_Y) / RADIO_HEIGHT);
                        	
                        	if (tY >= 0 && tY < templates.size())
                        		active_template = tY;
                        		
				if (tY == templates.size()+1 && x >= TEMPLATE_X + 4) {
					bool is_existing = false;
					for (auto e : projects) {
						if (e == name)
							is_existing = true;
					}
						
					if (is_existing)
						return true;
					create_project_from_template(name, templates[active_template]);
					return true;;
				}
                        }

                        if (x >= PROJECT_X && y > PROJECT_Y) {
                        	unsigned int tX = floor((x - PROJECT_X) / (PROJECT_WIDTH + 1));
                        	unsigned int tY = floor((y - PROJECT_Y) / RADIO_HEIGHT);
                        	unsigned int index;
                        	
                        	index = tY + tX * PER_COLUMN;
                        	printf("index is %i \n", index);
                        	
                        	
                        	if (index == active_project && time(NULL) - previous_click < 3) {
                        		/* double click */
					name = projects[active_project];
					return true;
                        	}
                        	if (index >= 0 && index < projects.size())
                        		active_project = index;

				/* click on the button */
				if (((tY == projects.size()+1) || (tY > PER_COLUMN+1))  && x >= PROJECT_X + 4) {
					name = projects[active_project];
					return true;;
				}
                        }
                        
                        previous_click = time(NULL);
                        break;
		case SDL_MOUSEMOTION:
                        currentX = scr_to_X(event.motion.x);
                        currentY = scr_to_Y(event.motion.y);
                        break;
	}
	
	newname->handle_event(event);
	
	return false;
}

void projcanvas::run(void)
{
	SDL_Event event = {};
	while (true) {
		int ret;
		draw();
		ret = SDL_WaitEventTimeout(&event, 5);
		if (!ret) {
			continue;
		}
	
		if (handle_event(event))
			break;		
	}
}

void projcanvas::crawl_filesystem(void)
{
    const std::filesystem::path projpath{"projects/"};
    const std::filesystem::path templatepath{"templates/"};
    
    if (std::filesystem::exists(templatepath)) {
	    for (auto const &dir_entry : std::filesystem::directory_iterator{templatepath}) {
	        std::string path = dir_entry.path().filename();
	        std::string fullpath = dir_entry.path();
        
	        if (!dir_entry.is_directory())
	            continue;
            
	        templates.push_back(path);
	        
	        std::ifstream file(fullpath + "/description");
	        if (!file.is_open()) {
	        	template_descriptions[path] = path;
	        	continue;
	        }
	        std::stringstream buffer;
	        buffer << file.rdbuf();
	        file.close();
	        template_descriptions[path] = buffer.str();
	     }
    }
    
    
    if (!std::filesystem::exists(projpath)) {
    	std::filesystem::create_directory("projects/");
        return;
    }
    
    std::string newest_project = "NONE";
    std::filesystem::file_time_type newest_time = {};
    bool first = true;
    
    for (auto const &dir_entry : std::filesystem::directory_iterator{projpath}) {
        std::string path = dir_entry.path().filename();
	std::string jsonstr = dir_entry.path();
        jsonstr = jsonstr + "/scene.json";
	std::filesystem::path jsonpath(jsonstr);
        
        if (std::filesystem::last_write_time(jsonpath) > newest_time || first) {
        	newest_time = std::filesystem::last_write_time(jsonpath);
        	newest_project = path;
        }
        first = false;
        
        if (!dir_entry.is_directory())
            continue;
            
        projects.push_back(path);
     }
     
     printf("newest project is %s \n", newest_project.c_str());
     
     for (unsigned int i = 0; i < projects.size() ; i++) {
     	if (projects[i] == newest_project)
     		active_project = i;
     }

}

void projcanvas::crawl_library(void)
{
    const std::filesystem::path libpath{"library/"};
    for (auto const &dir_entry : std::filesystem::recursive_directory_iterator{libpath}) {
        std::string path = dir_entry.path().filename();
        if (!path.ends_with(".json"))
        	continue;
        	
	if (path.starts_with("testbench_"))
		continue;
		
	projects.push_back(dir_entry.path());
        	
	printf("Library %s\n", path.c_str());
    }
    
}

void projcanvas::create_project_from_template(std::string name, std::string templ)
{
    	std::filesystem::create_directory("projects/" + name);
    	
    	std::string target = "projects/" + name + "/";
    	if (templ == "<None>") 
    		return;
	/* ok now we know we have a template that exists -- copy all the files over recursively */
	const std::filesystem::path templatepath{"templates/" + templ};
    
	if (std::filesystem::exists(templatepath)) {
	    for (auto const &dir_entry : std::filesystem::recursive_directory_iterator{templatepath}) {
	        std::string path = dir_entry.path().filename();
	        std::string fullpath = dir_entry.path();
 
	        std::string relative = fullpath.substr(("templates/"+templ).size() + 1, std::string::npos);
	        printf("Found %s / %s /%s \n", path.c_str(), fullpath.c_str(), relative.c_str());
	        
 	        if (dir_entry.is_directory()) {
 	        	printf("mkdir of %s\n", (target + path).c_str());
 	        	std::filesystem::create_directory(target + relative);
 	        } else {
		  std::ostringstream buf; 
		  std::ifstream input (fullpath); 
		  buf << input.rdbuf(); 
		  input.close();
	  	  std::ofstream output(target + relative,  std::ios::binary);
	  	  output << buf.str();
	  	  output.close();
 	        }
	}       
      }
}
