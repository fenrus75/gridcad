#include "gridcad.h"
#include "hexcanvas.h"


#include <SDL2/SDL.h>

#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_sdlrenderer2.h"
#include "imgui_hex.h"

hexcanvas::hexcanvas(std::string name, unsigned char *_buffer, unsigned int _length) : basecanvas()
{
    buffer = _buffer;
    length = _length;
    std::string s;
    s = "Content edit for " + name;

    window = SDL_CreateWindow(s.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1024, 768, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    windowID = SDL_GetWindowID(window);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);
}


hexcanvas::~hexcanvas(void)
{
}


bool hexcanvas::handle_event(SDL_Event &event)
{

    ImGui_ImplSDL2_ProcessEvent(&event);

    if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE) {
        SDL_HideWindow(window); 
        return false;
    }

    return false;
}


void hexcanvas::draw(void)
{
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
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
    SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
    SDL_RenderClear(renderer);
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
    SDL_RenderPresent(renderer);
}
