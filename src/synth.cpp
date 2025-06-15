#include "gridcad.h"

#include <cstdlib>

#include "synth.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <pthread.h>


static void *polling_thread(void * data)
{
        FILE *pipe = (FILE *)data;
        ssize_t ret;
        size_t size;
        char * line = NULL;

        while (pipe) {
            line = NULL;
            ret = getline(&line, &size, pipe);

        
        
            if (ret < 0) {
                pclose(pipe);
                pipe = NULL;
                break;
            } else {
                if (line) {
                	SDL_Event ev = {};
                	ev.type = EVENT_SYNTH_DATA;
                	ev.user.code = 0;
                	ev.user.data2 = line;
	
                	SDL_PushEvent(&ev);
                }
                
            }
        }        


     {
                	SDL_Event ev = {};
                	ev.type = EVENT_SYNTH_DONE;
                	ev.user.code = 0;
	
                	SDL_PushEvent(&ev);
     }
    
    if (pipe)
        pclose(pipe);
    pipe = NULL;
    return NULL;
}

synth::synth(int screenX, int screenY, std::string projectname) : dialog::dialog(screenX, screenY, "THIS IS THE MINIMUM WIDTH OF THE DIALOG BOX")
{
    std::string command;
    
    command = "make -C " + projectname + "/verilog ";
    pipe = popen(command.c_str(), "r");

    if (!pipe) {
        append_line("Failed to execute compilation steps");
        enable_ok_button();    
    } else {
        pthread_t pt;
        pthread_create(&pt, NULL, polling_thread, (void*)pipe);
    }
}

synth::~synth(void)
{
}

void synth::handle_event(class canvas *canvas, SDL_Event &event)
{
    dialog::handle_event(canvas, event);
    
    if (event.type == EVENT_SYNTH_DATA) {
        char *str;
        std::string s;
        str = (char *)event.user.data2;
        s = str;
        content.push_back(s);
        while (content.size() > 10)
            content.erase(content.begin());
    }
    if (event.type == EVENT_SYNTH_DONE) {
        enable_ok_button();
    }
    
}