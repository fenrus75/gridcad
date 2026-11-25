#include "gridcad.h"
#include "hexcanvas.h"


#include <SDL3/SDL.h>

#include "imgui.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlrenderer3.h"
#include "imgui_hex.h"

hexcanvas::hexcanvas(std::string name, unsigned char *_buffer, unsigned int _length) : basecanvas()
{
    buffer = _buffer;
    length = _length;
    std::string s;
    s = "Content edit for " + name;

    window.reset(SDL_CreateWindow(s.c_str(), 1024, 768, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY));
    renderer.reset(SDL_CreateRenderer(window.get(), nullptr));
    windowID = SDL_GetWindowID(window.get());
    SDL_SetRenderDrawBlendMode(renderer.get(), SDL_BLENDMODE_BLEND);

    ImGui_ImplSDL3_InitForSDLRenderer(window.get(), renderer.get());
    ImGui_ImplSDLRenderer3_Init(renderer.get());
}


hexcanvas::~hexcanvas(void)
{
    unregister_canvas(this);
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
}


bool hexcanvas::handle_event(SDL_Event &event)
{

    ImGui_ImplSDL3_ProcessEvent(&event);

    if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
        SDL_HideWindow(window.get()); 
        return false;
    }

    return false;
}


void hexcanvas::draw(void)
{
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    static ImGuiHexEditorState hex_state;
    hex_state.Bytes = buffer;
    hex_state.MaxBytes = length;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos); // 1.5.3
    ImGui::SetNextWindowSize(viewport->Size); // 1.5.3

    ImGui::Begin("Hex Editor", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);        
    ImGui::BeginHexEditor("##HexEditor", &hex_state);
    ImGui::EndHexEditor();
    ImGui::End();

    
    ImGui::Render();
    SDL_SetRenderScale(renderer.get(), io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
    SDL_RenderClear(renderer.get());
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer.get());
}
