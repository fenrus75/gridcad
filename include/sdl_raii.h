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
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

namespace sdl {

/*
 * Zero-overhead RAII smart pointer wrappers for SDL2 / SDL2_ttf handles.
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
            SDL_FreeSurface(s);
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
 * Return the target window ID for an SDL event without assuming union-member
 * layout aliasing (required for SDL3 portability).
 */
inline Uint32 event_window_id(const SDL_Event &ev) noexcept
{
    switch (ev.type) {
    case SDL_WINDOWEVENT:
        return ev.window.windowID;
    case SDL_KEYDOWN:
    case SDL_KEYUP:
        return ev.key.windowID;
    case SDL_TEXTEDITING:
        return ev.edit.windowID;
    case SDL_TEXTINPUT:
        return ev.text.windowID;
    case SDL_MOUSEMOTION:
        return ev.motion.windowID;
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
        return ev.button.windowID;
    case SDL_MOUSEWHEEL:
        return ev.wheel.windowID;
    case SDL_DROPFILE:
    case SDL_DROPTEXT:
    case SDL_DROPBEGIN:
    case SDL_DROPCOMPLETE:
        return ev.drop.windowID;
    default:
        if (ev.type >= SDL_USEREVENT)
            return ev.user.windowID;
        return 0;
    }
}

/*
 * Return display content scale factor (1.0 = 96 DPI, 2.0 = 192 DPI).
 * Maps directly to SDL_GetDisplayContentScale / SDL_GetWindowDisplayScale in SDL3.
 */
inline float primary_display_scale() noexcept
{
    float dpi = 96.0f;
    if (SDL_GetDisplayDPI(0, &dpi, nullptr, nullptr) != 0 || dpi < 96.0f)
        dpi = 96.0f;
    return dpi / 96.0f;
}

inline float window_display_scale(SDL_Window *window) noexcept
{
    int idx = window ? SDL_GetWindowDisplayIndex(window) : 0;
    if (idx < 0)
        idx = 0;
    float dpi = 96.0f;
    if (SDL_GetDisplayDPI(idx, &dpi, nullptr, nullptr) != 0 || dpi < 96.0f)
        dpi = 96.0f;
    return dpi / 96.0f;
}

} // namespace sdl

#if !SDL_VERSION_ATLEAST(3, 0, 0)
/* Temporary SDL3-compatible shim over SDL2's SDL_QueryTexture(). */
inline bool SDL_GetTextureSize(SDL_Texture *texture, float *w, float *h) noexcept
{
    int iw = 0, ih = 0;
    if (!texture || SDL_QueryTexture(texture, nullptr, nullptr, &iw, &ih) != 0)
        return false;
    if (w)
        *w = static_cast<float>(iw);
    if (h)
        *h = static_cast<float>(ih);
    return true;
}
#endif
