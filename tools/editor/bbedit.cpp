// bbedit - the level editor (migration.md Phase 8, docs/editor.md).
//
//   bbedit [source-dir] [level] [--at x zoom] [--show part]... [--screenshot file.bmp]
//   bbedit [source-dir] --self-test
//
// Edits the source levels (levels/<stem>.json) and the sprite banks'
// hotspots and names of a source asset tree; source-dir defaults to
// assets/source of the current directory or of one of its parents.
// --at shows the level from x at a zoom. --screenshot draws a few frames, saves the window to a BMP file and quits
// (a smoke test of the drawing). --self-test edits snuf1 through the
// interface with SDL input events and checks the result (self_test.hpp); it
// saves nothing.

#include <SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>

#include <cstdio>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "editor_app.hpp"
#include "self_test.hpp"

namespace fs = std::filesystem;

namespace {

std::string findSourceTree() {
    std::error_code ec;
    for (fs::path dir = fs::current_path(ec); !dir.empty(); dir = dir.parent_path()) {
        if (fs::is_directory(dir / "assets" / "source" / "levels", ec)) {
            return (dir / "assets" / "source").u8string();
        }
        if (dir == dir.parent_path()) {
            break;
        }
    }
    return {};
}

int run(int argc, char* argv[]) {
    std::string screenshot;
    float atX = -1;
    float atZoom = 1;
    bool selfTest = false;
    std::vector<std::string> shows;
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--screenshot" && i + 1 < argc) {
            screenshot = argv[++i];
        } else if (arg == "--show" && i + 1 < argc) {
            shows.emplace_back(argv[++i]);
        } else if (arg == "--self-test") {
            selfTest = true;
        } else if (arg == "--at" && i + 2 < argc) {
            atX = std::strtof(argv[i + 1], nullptr);
            atZoom = std::strtof(argv[i + 2], nullptr);
            i += 2;
        } else {
            args.push_back(arg);
        }
    }
    std::string sourceDir = !args.empty() ? args[0] : findSourceTree();
    if (sourceDir.empty() || !fs::is_directory(fs::u8path(sourceDir) / "levels")) {
        std::fprintf(stderr,
                     "usage: bbedit [source-dir] [level]\n"
                     "No source asset tree (a directory with levels/) was found%s%s.\n",
                     sourceDir.empty() ? "" : " at ", sourceDir.c_str());
        return 2;
    }

    if (selfTest) {
        // No screen needed, and no real mouse to mix with the test's.
        SDL_SetHint(SDL_HINT_VIDEODRIVER, "dummy");
        args.resize(1);
        args.emplace_back("snuf1");
        atX = 1350;
        atZoom = 1;
    }
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::fprintf(stderr, "bbedit: SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
    SDL_Window* window = SDL_CreateWindow("Blip & Blop level editor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1440,
                                          860, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (window == nullptr) {
        std::fprintf(stderr, "bbedit: SDL_CreateWindow: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == nullptr) {
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (renderer == nullptr) {
        std::fprintf(stderr, "bbedit: SDL_CreateRenderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    // High-DPI screens: draw at the screen's pixels, the interface at its
    // usual size.
    int windowWidth = 0;
    int outputWidth = 0;
    SDL_GetWindowSize(window, &windowWidth, nullptr);
    SDL_GetRendererOutputSize(renderer, &outputWidth, nullptr);
    const float scale = windowWidth > 0 ? static_cast<float>(outputWidth) / static_cast<float>(windowWidth) : 1.0f;
    SDL_RenderSetScale(renderer, scale, scale);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;  // the layout is fixed
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    int status = 0;
    std::string toolDir;
    if (char* base = SDL_GetBasePath()) {
        toolDir = base;
        SDL_free(base);
    }
    {
        bb::editor::EditorApp app(renderer, sourceDir, toolDir);
        if (args.size() > 1 && app.open(args[1]) && atX >= 0) {
            app.lookAt(atX, atZoom);
        }
        for (const std::string& part : shows) {
            app.show(part);
        }
        int frames = 0;
        std::optional<bb::editor::SelfTest> test;
        if (selfTest) {
            test.emplace(app, window);
        }
        while (!app.done()) {
            if (test && !test->beforeFrame()) {
                status = test->failures() == 0 ? 0 : 1;
                break;
            }
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                ImGui_ImplSDL2_ProcessEvent(&event);
                if (event.type == SDL_QUIT ||
                    (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
                     event.window.windowID == SDL_GetWindowID(window))) {
                    app.requestClose();
                }
            }
            ImGui_ImplSDLRenderer2_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            ImGui::NewFrame();
            app.frame();
            ImGui::Render();
            SDL_SetRenderDrawColor(renderer, 20, 20, 24, 255);
            SDL_RenderClear(renderer);
            ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
            // (Textures load while drawing: a few frames before the picture.)
            if (!screenshot.empty() && ++frames == 5) {
                int w = 0;
                int h = 0;
                SDL_GetRendererOutputSize(renderer, &w, &h);
                SDL_Surface* shot = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_ARGB8888);
                if (shot != nullptr &&
                    SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_ARGB8888, shot->pixels, shot->pitch) == 0) {
                    SDL_SaveBMP(shot, screenshot.c_str());
                }
                SDL_FreeSurface(shot);
                break;
            }
            SDL_RenderPresent(renderer);
        }
    }

    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return status;
}

}  // namespace

int main(int argc, char* argv[]) {
    try {
        return run(argc, argv);
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "bbedit: %s\n", ex.what());
        return 1;
    }
}
