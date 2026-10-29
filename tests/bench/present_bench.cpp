// Present-path benchmark (migration.md 3.14, D-08): how long it takes to
// show one 640x480 frame through each SDL 2 path, without vsync.
//
//   texture-per-frame  SDL_CreateTextureFromSurface + destroy (2014 port)
//   streaming-texture  SDL_UpdateTexture on one streaming texture (Phase 1)
//   window-surface     copy into SDL_GetWindowSurface + SDL_UpdateWindowSurface
//
// Opens a real window (the dummy driver would measure nothing useful).
// Not part of ctest. Usage: bb_present_bench [frames] [reverse]
// ("reverse" measures the paths in the opposite order, to expose order bias).

#include <SDL.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

namespace {

constexpr int kWidth = 640;
constexpr int kHeight = 480;

double nowMs() {
    return static_cast<double>(SDL_GetPerformanceCounter()) * 1000.0 /
           static_cast<double>(SDL_GetPerformanceFrequency());
}

// A frame in the game's pixel layout (0xAABBGGRR integers = ABGR8888),
// changed a little every frame so that nothing can be cached.
struct Frame {
    std::vector<std::uint32_t> pixels =
        std::vector<std::uint32_t>(static_cast<std::size_t>(kWidth) * kHeight);
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormatFrom(pixels.data(), kWidth, kHeight, 32, kWidth * 4,
                                                              SDL_PIXELFORMAT_ABGR8888);
    void update(int n) {
        for (int y = 0; y < kHeight; y += 8) {
            std::uint32_t* row = pixels.data() + static_cast<std::ptrdiff_t>(y) * kWidth;
            for (int x = 0; x < kWidth; ++x) {
                row[x] = 0xFF000000u | static_cast<std::uint32_t>((x + n) & 0xFF) |
                         (static_cast<std::uint32_t>((y + n) & 0xFF) << 8) |
                         (static_cast<std::uint32_t>(n & 0xFF) << 16);
            }
        }
    }
};

struct Result {
    std::string name;
    double mean = 0;
    double median = 0;
    double p95 = 0;
};

Result measure(const std::string& name, int frames, Frame& frame, const std::function<void()>& present) {
    for (int i = 0; i < 30; ++i) {  // warm-up
        frame.update(i);
        present();
        SDL_PumpEvents();
    }
    std::vector<double> times;
    times.reserve(static_cast<std::size_t>(frames));
    for (int i = 0; i < frames; ++i) {
        frame.update(i);
        const double t0 = nowMs();
        present();
        times.push_back(nowMs() - t0);
        SDL_PumpEvents();
    }
    std::sort(times.begin(), times.end());
    Result r;
    r.name = name;
    for (double t : times) {
        r.mean += t;
    }
    r.mean /= static_cast<double>(times.size());
    r.median = times[times.size() / 2];
    r.p95 = times[times.size() * 95 / 100];
    return r;
}

SDL_Window* openWindow() {
    SDL_Window* window = SDL_CreateWindow("present bench", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          kWidth, kHeight, SDL_WINDOW_SHOWN);
    if (window == nullptr) {
        std::fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        std::exit(1);
    }
    return window;
}

SDL_Renderer* openRenderer(SDL_Window* window) {
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);  // no vsync
    if (renderer == nullptr) {
        std::fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
        std::exit(1);
    }
    SDL_RenderSetLogicalSize(renderer, kWidth, kHeight);
    return renderer;
}

}  // namespace

int main(int argc, char* argv[]) {
    const int frames = argc > 1 ? std::max(10, static_cast<int>(std::strtol(argv[1], nullptr, 10))) : 600;
    const bool reverse = argc > 2 && std::strcmp(argv[2], "reverse") == 0;
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    SDL_version linked;
    SDL_GetVersion(&linked);
    std::printf("SDL %d.%d.%d, video driver %s, %d frames per path\n", linked.major, linked.minor,
                linked.patch, SDL_GetCurrentVideoDriver(), frames);

    Frame frame;
    std::vector<Result> results;

    auto rendererPaths = [&] {
        {
            SDL_Window* window = openWindow();
            SDL_Renderer* renderer = openRenderer(window);
            SDL_RendererInfo info;
            SDL_GetRendererInfo(renderer, &info);
            std::printf("renderer: %s\n", info.name);
            auto perFrame = [&] {
                results.push_back(measure("texture-per-frame", frames, frame, [&] {
                    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, frame.surface);
                    SDL_RenderClear(renderer);
                    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
                    SDL_RenderPresent(renderer);
                    SDL_DestroyTexture(texture);
                }));
            };

            SDL_Texture* streaming = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888,
                                                       SDL_TEXTUREACCESS_STREAMING, kWidth, kHeight);
            SDL_SetTextureBlendMode(streaming, SDL_BLENDMODE_NONE);
            auto streamingPath = [&] {
                results.push_back(measure("streaming-texture", frames, frame, [&] {
                    SDL_UpdateTexture(streaming, nullptr, frame.pixels.data(), kWidth * 4);
                    SDL_RenderClear(renderer);
                    SDL_RenderCopy(renderer, streaming, nullptr, nullptr);
                    SDL_RenderPresent(renderer);
                }));
            };

            if (reverse) {
                streamingPath();
                perFrame();
            } else {
                perFrame();
                streamingPath();
            }
            SDL_DestroyTexture(streaming);
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
        }
    };
    auto windowSurfacePath = [&] {
        {
            // A window with a renderer cannot use the window surface: new window.
            SDL_Window* window = openWindow();
            SDL_Surface* target = SDL_GetWindowSurface(window);
            if (target == nullptr) {
                std::fprintf(stderr, "SDL_GetWindowSurface: %s\n", SDL_GetError());
                std::exit(1);
            }
            std::printf("window surface: %dx%d %s\n", target->w, target->h,
                        SDL_GetPixelFormatName(target->format->format));
            SDL_SetSurfaceBlendMode(frame.surface, SDL_BLENDMODE_NONE);
            results.push_back(measure("window-surface", frames, frame, [&] {
                SDL_BlitSurface(frame.surface, nullptr, target, nullptr);  // converts to the window format
                SDL_UpdateWindowSurface(window);
            }));
            SDL_DestroyWindow(window);
        }
    };
    if (reverse) {
        windowSurfacePath();
        rendererPaths();
    } else {
        rendererPaths();
        windowSurfacePath();
    }

    std::printf("%-20s %10s %10s %10s\n", "path", "mean ms", "median ms", "p95 ms");
    for (const Result& r : results) {
        std::printf("%-20s %10.3f %10.3f %10.3f\n", r.name.c_str(), r.mean, r.median, r.p95);
    }
    SDL_FreeSurface(frame.surface);
    SDL_Quit();
    return 0;
}
