#include "gridcad.h"

#include <cstdlib>
#include <regex>

#include "synth.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <pthread.h>


/* 0 = start
   1 = yosys
   
   100 = finished */
static int phase = 0;


std::string synth::log2string(char *str)
{
    char *c;
    c = strchr(str, '\n');
    if (c) *c = 0;
    
    std::string s = str;
    if (phase == 0) {
        if (strstr(str, "yosys"))
            phase = 1;
    }
        
    if (phase == 1) {
        if (strstr(str, "Time spent:"))
            phase = 2;
        if (str[0] >= '0' && str[0] <= '9' && false) {
            /* new main line -- delete all non-main lines from the history */
                bool retry = true;
                while (retry) {
                    retry = false;
                    for (unsigned int i = 0; i < content.size(); i++) {
                        std::string l = content[i];
                        if (l[0] < '0' || l[0] > '9') {
                            content.erase(content.begin() + i);
                            retry = true;
                            break;
                        }
                    }
                }
        }
    }
    
    
    return s;
}


static void *polling_thread(void * data)
{
        FILE *pipe = (FILE *)data;
        std::string current_line = "";
        

        while (pipe) {
            int ret;
            char c;
            ret = fgetc(pipe);
            
            if (ret < 0) {
                pclose(pipe);
                pipe = NULL;
                break;
            };
            c = ret;
            
            if (c == '\n' || c == 0xd) {
                    char *line = strdup(current_line.c_str());
                    SDL_Event ev = {};
                    ev.type = EVENT_SYNTH_DATA;
                    ev.user.code = 0;
                    ev.user.data2 = line;
	
                    SDL_PushEvent(&ev);
                    current_line = "";
                    if (c == 0xd)
                        current_line = c;
            } else {
                current_line = current_line + c;
            }
                
        }        


        SDL_Event ev = {};
        ev.type = EVENT_SYNTH_DONE;
        ev.user.code = 0;
	
        SDL_PushEvent(&ev);
    
        if (pipe)
            pclose(pipe);
        pipe = NULL;
        return NULL;
}

synth::synth(int screenX, int screenY, std::string projectname, std::string make_target) : dialog::dialog(screenX, screenY, "THIS IS THE MINIMUM WIDTH OF THE DIALOG BOX")
{
    std::string command;
    
    command = "make -C " + projectname + "/verilog " + make_target + " 2>&1";
    pipe = popen(command.c_str(), "r");

    phase = 0;
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

void synth::handle_event(class basecanvas *canvas, SDL_Event &event)
{
    dialog::handle_event(canvas, event);
    
    if (event.type == EVENT_SYNTH_DATA) {
        char *str;
        std::string s;
        str = (char *)event.user.data2;
        s = log2string(str);
        
        if (s.starts_with("Info: Max frequency for clock "))
            frequency_line = s;
        if (s != "") {
            char c = 0xd;
            if (s[0] == c)
                content[content.size() - 1] = s;
            else
                content.push_back(s);
        }
        free(str);
        while (content.size() > 10)
            content.erase(content.begin());
    }
    if (event.type == EVENT_SYNTH_DONE) {
        if (frequency_line != "")
            content.push_back(frequency_line);
        enable_ok_button();
    }
    
}