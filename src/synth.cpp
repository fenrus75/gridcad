#include "gridcad.h"

#include <cstdlib>
#include <memory>
#include <regex>

#include "synth.h"

namespace {

struct PipeDeleter {
    void operator()(FILE *f) const noexcept {
        if (f)
            pclose(f);
    }
};

using PipePtr = std::unique_ptr<FILE, PipeDeleter>;

} // namespace


std::string synth::log2string(std::string_view str)
{
    auto pos = str.find('\n');
    if (pos != std::string_view::npos) {
        str = str.substr(0, pos);
    }
    
    std::string s(str);
    if (phase == 0) {
        if (str.find("yosys") != std::string::npos)
            phase = 1;
    }
        
    if (phase == 1) {
        if (str.find("Time spent:") != std::string::npos)
            phase = 2;
        if (!str.empty() && str[0] >= '0' && str[0] <= '9' && false) {
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

synth::synth(int screenX, int screenY, std::string projectname, std::string make_target) : dialog::dialog(screenX, screenY, "THIS IS THE MINIMUM WIDTH OF THE DIALOG BOX")
{
    std::string command = "make -C " + projectname + "/verilog " + make_target + " 2>&1";
    PipePtr pipe(popen(command.c_str(), "r"));

    phase = 0;
    if (!pipe) {
        append_line("Failed to execute compilation steps");
        enable_ok_button();
        return;
    }

    worker = std::jthread([this, pipe = std::move(pipe)](std::stop_token stoken) mutable {
        std::string current_line;

        while (pipe && !stoken.stop_requested()) {
            int ret = fgetc(pipe.get());
            if (ret < 0)
                break;

            char c = static_cast<char>(ret);
            if (c == '\n' || c == '\r') {
                {
                    std::lock_guard<std::mutex> lock(queue_mutex);
                    pending_lines.push(std::move(current_line));
                }
                SDL_Event ev = {};
                ev.type = EVENT_SYNTH_DATA;
                ev.user.code = 0;
                SDL_PushEvent(&ev);

                current_line.clear();
                if (c == '\r')
                    current_line.push_back(c);
            } else {
                current_line.push_back(c);
            }
        }

        pipe.reset();

        if (!stoken.stop_requested()) {
            SDL_Event ev = {};
            ev.type = EVENT_SYNTH_DONE;
            ev.user.code = 0;
            SDL_PushEvent(&ev);
        }
    });
}

void synth::handle_event(class basecanvas *canvas, SDL_Event &event)
{
    dialog::handle_event(canvas, event);
    
    if (event.type == EVENT_SYNTH_DATA) {
        std::queue<std::string> batch;
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            std::swap(batch, pending_lines);
        }

        while (!batch.empty()) {
            std::string s = log2string(std::move(batch.front()));
            batch.pop();

            if (s.starts_with("Info: Max frequency for clock "))
                frequency_line = s;
            if (!s.empty()) {
                if (s[0] == '\r' && !content.empty())
                    content.back() = s;
                else
                    content.push_back(s);
            }
            while (content.size() > 10)
                content.erase(content.begin());
        }
    }
    if (event.type == EVENT_SYNTH_DONE) {
        if (!frequency_line.empty())
            content.push_back(frequency_line);
        enable_ok_button();
    }
}