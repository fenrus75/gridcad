/*
 * gridcad -- a tool to do digital circuit design with a GUI
 *
 * (C) Copyright 2025  -- Arjan van de Ven
 *
 * Licensed under the terms of the
 * GNU General Public LIcense version 3.0
 *
 * Authors:
 *    Arjan van de Ven <arjanvandeven@gmail.com>
 */

#pragma once

#include <memory>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

namespace sdl {

/*
 * Zero-overhead RAII smart pointer wrappers for SDL3 / SDL3_ttf handles.
 * Destruction order when held as class members follows standard C++ reverse
 * declaration order: declare WindowPtr before RendererPtr before any
 * TexturePtr / FontPtr caches so textures are freed while the renderer is
 * still valid.
 */

struct WindowDeleter {
    void operator()(SDL_Window *w) const noexcept {
        if (w)
            SDL_DestroyWindow(w);
    }
};

struct RendererDeleter {
    void operator()(SDL_Renderer *r) const noexcept {
        if (r)
            SDL_DestroyRenderer(r);
    }
};

struct TextureDeleter {
    void operator()(SDL_Texture *t) const noexcept {
        if (t)
            SDL_DestroyTexture(t);
    }
};

struct SurfaceDeleter {
    void operator()(SDL_Surface *s) const noexcept {
        if (s)
            SDL_DestroySurface(s);
    }
};

struct FontDeleter {
    void operator()(TTF_Font *f) const noexcept {
        if (f)
            TTF_CloseFont(f);
    }
};

using WindowPtr   = std::unique_ptr<SDL_Window, WindowDeleter>;
using RendererPtr = std::unique_ptr<SDL_Renderer, RendererDeleter>;
using TexturePtr  = std::unique_ptr<SDL_Texture, TextureDeleter>;
using SurfacePtr  = std::unique_ptr<SDL_Surface, SurfaceDeleter>;
using FontPtr     = std::unique_ptr<TTF_Font, FontDeleter>;

/*
 * RAII wrapper for SDL_TimerID (removes timer on destruction or reset).
 */
class Timer {
public:
    Timer() noexcept = default;
    Timer(Uint32 interval, SDL_TimerCallback callback, void *param = nullptr) noexcept
        : id_(SDL_AddTimer(interval, callback, param)) {}

    ~Timer() noexcept { reset(); }

    Timer(const Timer &) = delete;
    Timer &operator=(const Timer &) = delete;

    Timer(Timer &&other) noexcept : id_(other.id_) { other.id_ = 0; }
    Timer &operator=(Timer &&other) noexcept {
        if (this != &other) {
            reset();
            id_ = other.id_;
            other.id_ = 0;
        }
        return *this;
    }

    void reset(SDL_TimerID new_id = 0) noexcept {
        if (id_ != 0)
            SDL_RemoveTimer(id_);
        id_ = new_id;
    }

    explicit operator bool() const noexcept { return id_ != 0; }

private:
    SDL_TimerID id_ = 0;
};

/*
 * Process-wide RAII guard for SDL2, SDL2_ttf, and ImGui context lifetime.
 * Instantiate once at top-level scope (e.g. main()) so all windows, renderers,
 * textures, and timers are destroyed before SDL_Quit().
 */
class Context {
public:
    Context();
    ~Context();

    Context(const Context &) = delete;
    Context &operator=(const Context &) = delete;
    Context(Context &&) = delete;
    Context &operator=(Context &&) = delete;
};

/*
 * Return the target window ID for an SDL event using SDL3's SDL_GetWindowFromEvent().
 */
inline SDL_WindowID event_window_id(const SDL_Event &ev) noexcept
{
    SDL_Window *w = SDL_GetWindowFromEvent(&ev);
    return w ? SDL_GetWindowID(w) : 0;
}

/*
 * Return display content scale factor (1.0 = 96 DPI, 2.0 = 192 DPI).
 */
inline float primary_display_scale() noexcept
{
    SDL_DisplayID d = SDL_GetPrimaryDisplay();
    float s = d ? SDL_GetDisplayContentScale(d) : 1.0f;
    return (s > 0.0f) ? s : 1.0f;
}

inline float window_display_scale(SDL_Window *window) noexcept
{
    float s = window ? SDL_GetWindowDisplayScale(window) : primary_display_scale();
    return (s > 0.0f) ? s : 1.0f;
}

} // namespace sdl
