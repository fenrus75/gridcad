/*
 * gridcad -- a tool to do digital circuit design with a GUI
 *
 * (C) Copyright 2025  -- Arjan van de Ven
 *
 * Licensed under the terms of the
 * GNU General Public LIcense version 3.0
 */

#pragma once

#include <string_view>

/*
================================================================================
                    CRASH HANDLER ASYNC-SIGNAL SAFETY RULES
================================================================================
Functions in this subsystem fall into two categories:

1. [CRITICAL: MUST BE STRICTLY ASYNC-SIGNAL-SAFE]
   Any code invoked during crash signal handling:
   - NO dynamic memory allocations (malloc/free/new/delete).
   - NO std::string, std::vector, or any heap-allocating container.
   - NO stdio or formatting streams (printf, sprintf, snprintf, cout).
   - NO mutexes, condition variables, or locks (deadlock hazard).
   - ONLY fixed-size stack buffers, safe stack-only formatters, and POSIX
     syscalls (open, close, read, write, fork, execl, kill, _exit).

2. [INIT / NORMAL CONTEXT ONLY: NOT ASYNC-SIGNAL-SAFE]
   Functions executed at initialization (e.g. install_fallback_handler,
   setup_crash_file) or normal user thread breadcrumb registration.
================================================================================
*/

namespace crash_handler
{

// [INIT ONLY: NOT ASYNC-SIGNAL-SAFE]
// Installs fallback crash signal handlers (with alternate signal stack) and
// std::set_terminate handler if no debugger or custom handler is active.
void install_fallback_handler();

// [INIT ONLY: NOT ASYNC-SIGNAL-SAFE]
// Checks /proc/self/status for TracerPid != 0 to detect if GDB is attached.
bool is_debugger_attached();

// [CRITICAL: ASYNC-SIGNAL-SAFE HANDLE]
// Open file descriptor for the active crash log, or -1 if deferred until crash.
extern int crash_fd;

// [NORMAL CONTEXT: THREAD-SAFE / SIGNAL-SAFE READ]
// Sets or clears the thread-local breadcrumb for crash reporting.
void set_breadcrumb(std::string_view breadcrumb);
void clear_breadcrumb();

// RAII helper to manage thread-local breadcrumb lifecycle.
class scoped_breadcrumb {
public:
    explicit scoped_breadcrumb(std::string_view breadcrumb);
    ~scoped_breadcrumb();

    scoped_breadcrumb(const scoped_breadcrumb &) = delete;
    scoped_breadcrumb &operator=(const scoped_breadcrumb &) = delete;
    scoped_breadcrumb(scoped_breadcrumb &&) = delete;
    scoped_breadcrumb &operator=(scoped_breadcrumb &&) = delete;

private:
    char prev_[128];
};

} // namespace crash_handler
