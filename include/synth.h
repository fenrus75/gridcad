#pragma once

#include "gridcad.h"
#include "dialog.h"

#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <queue>
#include <thread>


class synth : public dialog
{
public:
    synth(int screenX, int screenY, std::string projectname, std::string maketarget);
    void handle_event(class basecanvas *canvas, SDL_Event &event) override;
    
protected:
    std::string log2string(std::string_view str);
    std::string frequency_line = "";
    int phase = 0;

    /*
     * Protects: pending_lines.
     * Locking rules: Acquired briefly by the background worker thread when
     * enqueuing output lines and by the UI thread in handle_event() when
     * draining pending_lines. Never held across I/O or SDL calls; no nested
     * locks are taken while holding queue_mutex.
     */
    std::mutex queue_mutex;
    std::queue<std::string> pending_lines;

    /* Declared last so the worker thread is requested to stop and joined
     * before queue_mutex and pending_lines are destroyed. */
    std::jthread worker;
};