#include "bb/platform/sdl2/sdl2_platform.hpp"

#include <SDL.h>
#include <SDL_mixer.h>

#if defined(_WIN32)
// clang-format off
#include <windows.h>
#include <psapi.h>
// clang-format on
#else
#include <sys/resource.h>
#endif

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace bb::platform::sdl2 {

namespace {

class Sdl2Clock final : public Clock {
public:
    Sdl2Clock() : start_(SDL_GetPerformanceCounter()), frequency_(SDL_GetPerformanceFrequency()) {}

    std::uint64_t microseconds() const override {
        const std::uint64_t ticks = SDL_GetPerformanceCounter() - start_;
        return ticks / frequency_ * 1000000u + (ticks % frequency_) * 1000000u / frequency_;
    }

    void sleep(std::uint64_t microseconds) override {
        SDL_Delay(static_cast<Uint32>((microseconds + 999u) / 1000u));
    }

private:
    std::uint64_t start_;
    std::uint64_t frequency_;
};

class Sdl2System final : public System {
public:
    explicit Sdl2System(bool headless) : headless_(headless) {}

    void showError(const std::string& title, const std::string& message) override {
        show(SDL_MESSAGEBOX_ERROR, title, message);
    }

    void showWarning(const std::string& title, const std::string& message) override {
        show(SDL_MESSAGEBOX_WARNING, title, message);
    }

    std::uint64_t peakMemoryBytes() const override {
#if defined(_WIN32)
        PROCESS_MEMORY_COUNTERS counters;
        if (GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters))) {
            return counters.PeakWorkingSetSize;
        }
        return 0;
#else
        rusage usage{};
        if (getrusage(RUSAGE_SELF, &usage) != 0) {
            return 0;
        }
#if defined(__APPLE__)
        return static_cast<std::uint64_t>(usage.ru_maxrss);  // bytes on macOS
#else
        return static_cast<std::uint64_t>(usage.ru_maxrss) * 1024;  // kilobytes on Linux
#endif
#endif
    }

    std::string baseDirectory() const override {
        char* path = SDL_GetBasePath();
        if (path == nullptr) {
            return {};
        }
        std::string result(path);
        SDL_free(path);
        return result;
    }

    std::string preferencesDirectory() const override {
        char* path = SDL_GetPrefPath("LOADED Studio", "Blip and Blop");
        if (path == nullptr) {
            return {};
        }
        std::string result(path);
        SDL_free(path);
        return result;
    }

private:
    void show(Uint32 flags, const std::string& title, const std::string& message) const {
        if (!headless_ && SDL_ShowSimpleMessageBox(flags, title.c_str(), message.c_str(), nullptr) != 0) {
            SDL_Log("%s: %s (no message box: %s)", title.c_str(), message.c_str(), SDL_GetError());
        }
    }

    bool headless_;
};

class Sdl2Input final : public InputSource {
public:
    static constexpr int kMaxPads = 8;

    Sdl2Input() {
        SDL_JoystickEventState(SDL_ENABLE);
        SDL_GameControllerEventState(SDL_ENABLE);
        // Pads already connected at start-up get slots 0, 1, ... in SDL's
        // device order; they also arrive as SDL_JOYDEVICEADDED events.
        for (int i = 0; i < SDL_NumJoysticks(); ++i) {
            open(i);
        }
    }

    ~Sdl2Input() override {
        for (auto& pad : pads_) {
            close(pad);
        }
    }

    bool poll(InputEvent& out) override {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_QUIT:
                    out = {};
                    out.type = InputEvent::Type::Quit;
                    return true;
                case SDL_APP_WILLENTERBACKGROUND:
                case SDL_APP_DIDENTERFOREGROUND:
                    // Suspend and resume (mobile and console systems).
                    out = {};
                    out.type = e.type == SDL_APP_WILLENTERBACKGROUND ? InputEvent::Type::FocusLost
                                                                     : InputEvent::Type::FocusGained;
                    return true;
                case SDL_WINDOWEVENT:
                    if (e.window.event == SDL_WINDOWEVENT_FOCUS_LOST ||
                        e.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
                        out = {};
                        out.type = e.window.event == SDL_WINDOWEVENT_FOCUS_LOST
                                       ? InputEvent::Type::FocusLost
                                       : InputEvent::Type::FocusGained;
                        return true;
                    }
                    break;
                case SDL_KEYDOWN:
                case SDL_KEYUP:
                    out = {};
                    out.type = e.type == SDL_KEYDOWN ? InputEvent::Type::KeyDown : InputEvent::Type::KeyUp;
                    out.code = core::input::fromSdl2Key(e.key.keysym.sym);
                    return true;
                case SDL_JOYDEVICEADDED: {
                    const int slot = open(e.jdevice.which);
                    if (slot >= 0) {
                        out = {};
                        out.type = InputEvent::Type::PadAdded;
                        out.pad = slot;
                        return true;
                    }
                    break;
                }
                case SDL_JOYDEVICEREMOVED: {
                    const int slot = slotOf(e.jdevice.which);
                    if (slot >= 0) {
                        close(pads_[slot]);
                        out = {};
                        out.type = InputEvent::Type::PadRemoved;
                        out.pad = slot;
                        return true;
                    }
                    break;
                }
                case SDL_JOYBUTTONDOWN:
                case SDL_JOYBUTTONUP: {
                    const int slot = joystickSlotOf(e.jbutton.which);
                    if (slot >= 0) {
                        out = {};
                        out.type = e.type == SDL_JOYBUTTONDOWN ? InputEvent::Type::PadButtonDown
                                                               : InputEvent::Type::PadButtonUp;
                        out.pad = slot;
                        out.code = e.jbutton.button;
                        return true;
                    }
                    break;
                }
                case SDL_JOYAXISMOTION: {
                    const int slot = joystickSlotOf(e.jaxis.which);
                    if (slot >= 0) {
                        out = {};
                        out.type = InputEvent::Type::PadAxis;
                        out.pad = slot;
                        out.axis = e.jaxis.axis;
                        out.value = e.jaxis.value;
                        return true;
                    }
                    break;
                }
                case SDL_JOYHATMOTION: {
                    const int slot = joystickSlotOf(e.jhat.which);
                    if (slot >= 0) {
                        out = {};
                        out.type = InputEvent::Type::PadHat;
                        out.pad = slot;
                        out.axis = e.jhat.hat;
                        out.value = (e.jhat.value & SDL_HAT_UP ? InputEvent::kHatUp : 0) |
                                    (e.jhat.value & SDL_HAT_RIGHT ? InputEvent::kHatRight : 0) |
                                    (e.jhat.value & SDL_HAT_DOWN ? InputEvent::kHatDown : 0) |
                                    (e.jhat.value & SDL_HAT_LEFT ? InputEvent::kHatLeft : 0);
                        return true;
                    }
                    break;
                }
                // Game controllers: SDL's button numbers (codes.hpp), the
                // D-pad as a hat and the left stick as axes 0 and 1.
                case SDL_CONTROLLERBUTTONDOWN:
                case SDL_CONTROLLERBUTTONUP: {
                    const int slot = controllerSlotOf(e.cbutton.which);
                    if (slot < 0) {
                        break;
                    }
                    const bool down = e.type == SDL_CONTROLLERBUTTONDOWN;
                    if (const int bit = dpadBit(e.cbutton.button); bit != 0) {
                        Pad& pad = pads_[slot];
                        pad.dpad = down ? (pad.dpad | bit) : (pad.dpad & ~bit);
                        out = {};
                        out.type = InputEvent::Type::PadHat;
                        out.pad = slot;
                        out.value = pad.dpad;
                        return true;
                    }
                    out = {};
                    out.type = down ? InputEvent::Type::PadButtonDown : InputEvent::Type::PadButtonUp;
                    out.pad = slot;
                    out.code = e.cbutton.button;
                    return true;
                }
                case SDL_CONTROLLERAXISMOTION: {
                    const int slot = controllerSlotOf(e.caxis.which);
                    if (slot >= 0 && (e.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX ||
                                      e.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY)) {
                        out = {};
                        out.type = InputEvent::Type::PadAxis;
                        out.pad = slot;
                        out.axis = e.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX ? 0 : 1;
                        out.value = e.caxis.value;
                        return true;
                    }
                    break;
                }
                default:
                    break;
            }
        }
        return false;
    }

    int padSlots() const override { return slots_; }
    bool padConnected(int slot) const override {
        return slot >= 0 && slot < kMaxPads && pads_[slot].handle != nullptr;
    }
    std::string padName(int slot) const override {
        return padConnected(slot) ? pads_[slot].name : std::string();
    }
    bool padIsController(int slot) const override {
        return padConnected(slot) && pads_[slot].controller != nullptr;
    }

private:
    struct Pad {
        SDL_Joystick* handle = nullptr;
        SDL_GameController* controller = nullptr;  // for a recognised game controller
        SDL_JoystickID id = -1;
        std::string name;
        int dpad = 0;  // InputEvent::kHat* bits of the controller's D-pad
    };

    static int dpadBit(Uint8 button) {
        switch (button) {
            case SDL_CONTROLLER_BUTTON_DPAD_UP:
                return InputEvent::kHatUp;
            case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                return InputEvent::kHatRight;
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                return InputEvent::kHatDown;
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
                return InputEvent::kHatLeft;
            default:
                return 0;
        }
    }

    static void close(Pad& pad) {
        if (pad.controller != nullptr) {
            SDL_GameControllerClose(pad.controller);
        } else if (pad.handle != nullptr) {
            SDL_JoystickClose(pad.handle);
        }
        pad = Pad{};
    }

    // The slot of a pad whose joystick events are used (not a game
    // controller, whose controller events are used instead), or -1.
    int joystickSlotOf(SDL_JoystickID id) const {
        const int slot = slotOf(id);
        return slot >= 0 && pads_[slot].controller == nullptr ? slot : -1;
    }

    int controllerSlotOf(SDL_JoystickID id) const {
        const int slot = slotOf(id);
        return slot >= 0 && pads_[slot].controller != nullptr ? slot : -1;
    }

    int slotOf(SDL_JoystickID id) const {
        for (int i = 0; i < kMaxPads; ++i) {
            if (pads_[i].handle != nullptr && pads_[i].id == id) {
                return i;
            }
        }
        return -1;
    }

    // Opens device `index` in the first free slot, unless it is already open.
    int open(int index) {
        const SDL_JoystickID id = SDL_JoystickGetDeviceInstanceID(index);
        if (slotOf(id) >= 0) {
            return -1;
        }
        for (int i = 0; i < kMaxPads; ++i) {
            if (pads_[i].handle == nullptr) {
                // A pad SDL knows the layout of (its mapping database) is
                // opened as a game controller, others as a joystick.
                SDL_GameController* controller =
                    SDL_IsGameController(index) ? SDL_GameControllerOpen(index) : nullptr;
                SDL_Joystick* handle = controller != nullptr ? SDL_GameControllerGetJoystick(controller)
                                                             : SDL_JoystickOpen(index);
                if (handle == nullptr) {
                    return -1;
                }
                const char* name =
                    controller != nullptr ? SDL_GameControllerName(controller) : SDL_JoystickName(handle);
                pads_[i] =
                    Pad{handle, controller, SDL_JoystickInstanceID(handle), name != nullptr ? name : "", 0};
                if (i + 1 > slots_) {
                    slots_ = i + 1;
                }
                return i;
            }
        }
        return -1;
    }

    Pad pads_[kMaxPads];
    int slots_ = 0;
};

class Sdl2Audio final : public Audio {
public:
    // The 2002 game asked FMOD for 32 software channels (CHANNEL_NUMBER).
    // The 2014 port never called Mix_AllocateChannels, so it had SDL_mixer's
    // default of 8 voices.
    static constexpr int kVoices = 32;

    Sdl2Audio() {
        // The music is Ogg Vorbis, plus two MP3 jingles (tambour, gameover).
        const int wanted = MIX_INIT_OGG | MIX_INIT_MP3;
        if ((Mix_Init(wanted) & wanted) != wanted) {
            SDL_Log("Mix_Init: Ogg Vorbis or MP3 support missing, some music will be silent: %s",
                    Mix_GetError());
        }
        if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024) != 0) {
            SDL_Log("Mix_OpenAudio: %s", Mix_GetError());
            return;
        }
        Mix_AllocateChannels(kVoices);
        open_ = true;
    }

    ~Sdl2Audio() override {
        if (open_) {
            Mix_HaltChannel(-1);
            Mix_HaltMusic();
        }
        for (Mix_Chunk* chunk : samples_) {
            if (chunk != nullptr) {
                Mix_FreeChunk(chunk);
            }
        }
        for (MusicSlot& slot : musics_) {
            if (slot.music != nullptr) {
                Mix_FreeMusic(slot.music);
            }
        }
        if (open_) {
            Mix_CloseAudio();
        }
        while (Mix_Init(0) != 0) {
            Mix_Quit();
        }
    }

    bool available() const override { return open_; }

    Sample loadSample(const std::uint8_t* data, std::size_t size) override {
        if (!open_ || data == nullptr || size == 0 || size > 0x7FFFFFFFu) {
            return -1;
        }
        Mix_Chunk* chunk = Mix_LoadWAV_RW(SDL_RWFromConstMem(data, static_cast<int>(size)), 1);
        if (chunk == nullptr) {
            SDL_Log("Mix_LoadWAV_RW: %s", Mix_GetError());
            return -1;
        }
        return store(samples_, chunk);
    }

    void freeSample(Sample sample) override {
        if (Mix_Chunk* chunk = get(samples_, sample)) {
            // A chunk must not be freed while a voice plays it.
            for (int v = 0; v < kVoices; ++v) {
                if (Mix_GetChunk(v) == chunk && Mix_Playing(v) != 0) {
                    Mix_HaltChannel(v);
                }
            }
            Mix_FreeChunk(chunk);
            samples_[static_cast<std::size_t>(sample)] = nullptr;
        }
    }

    int play(Sample sample, bool loop) override {
        Mix_Chunk* chunk = get(samples_, sample);
        return chunk == nullptr ? -1 : Mix_PlayChannel(-1, chunk, loop ? -1 : 0);
    }

    void stop(int voice) override {
        if (open_ && voice >= 0) {
            Mix_HaltChannel(voice);
        }
    }

    Music openMusic(std::vector<std::uint8_t> file) override {
        if (!open_ || file.empty() || file.size() > 0x7FFFFFFFu) {
            return -1;
        }
        // SDL_mixer streams from the bytes, so they live as long as the music.
        MusicSlot slot;
        slot.file = std::make_unique<std::vector<std::uint8_t>>(std::move(file));
        slot.music =
            Mix_LoadMUS_RW(SDL_RWFromConstMem(slot.file->data(), static_cast<int>(slot.file->size())), 1);
        if (slot.music == nullptr) {
            SDL_Log("Mix_LoadMUS_RW: %s", Mix_GetError());
            return -1;
        }
        for (std::size_t i = 0; i < musics_.size(); ++i) {
            if (musics_[i].music == nullptr) {
                musics_[i] = std::move(slot);
                return static_cast<int>(i);
            }
        }
        musics_.push_back(std::move(slot));
        return static_cast<int>(musics_.size() - 1);
    }

    void freeMusic(Music music) override {
        if (MusicSlot* slot = musicSlot(music)) {
            Mix_FreeMusic(slot->music);  // halts it if it is playing
            *slot = MusicSlot();
        }
    }

    void playMusic(Music music, bool loop) override {
        if (MusicSlot* slot = musicSlot(music)) {
            Mix_PlayMusic(slot->music, loop ? -1 : 0);
        }
    }

    void stopMusic() override {
        if (open_) {
            Mix_HaltMusic();
        }
    }

private:
    template <typename T>
    static int store(std::vector<T*>& slots, T* item) {
        for (std::size_t i = 0; i < slots.size(); ++i) {
            if (slots[i] == nullptr) {
                slots[i] = item;
                return static_cast<int>(i);
            }
        }
        slots.push_back(item);
        return static_cast<int>(slots.size() - 1);
    }

    template <typename T>
    static T* get(const std::vector<T*>& slots, int handle) {
        return handle >= 0 && static_cast<std::size_t>(handle) < slots.size()
                   ? slots[static_cast<std::size_t>(handle)]
                   : nullptr;
    }

    struct MusicSlot {
        Mix_Music* music = nullptr;
        std::unique_ptr<std::vector<std::uint8_t>> file;
    };

    MusicSlot* musicSlot(Music music) {
        if (music < 0 || static_cast<std::size_t>(music) >= musics_.size()) {
            return nullptr;
        }
        MusicSlot& slot = musics_[static_cast<std::size_t>(music)];
        return slot.music != nullptr ? &slot : nullptr;
    }

    bool open_ = false;
    std::vector<Mix_Chunk*> samples_;
    std::vector<MusicSlot> musics_;
};

class Sdl2Display final : public Display {
public:
    Sdl2Display(SDL_Window* window, SDL_Renderer* renderer, SDL_Texture* texture, int width, int height)
        : window_(window), renderer_(renderer), texture_(texture), width_(width), height_(height) {
        SDL_RendererInfo info;
        vsync_ = renderer_ != nullptr && SDL_GetRendererInfo(renderer_, &info) == 0 &&
                 (info.flags & SDL_RENDERER_PRESENTVSYNC) != 0;
    }

    ~Sdl2Display() override {
        if (texture_ != nullptr) {
            SDL_DestroyTexture(texture_);
        }
        if (renderer_ != nullptr) {
            SDL_DestroyRenderer(renderer_);
        }
        if (window_ != nullptr) {
            SDL_DestroyWindow(window_);
        }
    }

    void present(const core::gfx::Pixmap& frame) override {
        if (renderer_ == nullptr || frame.width() != width_ || frame.height() != height_) {
            return;
        }
        // One upload per frame; the frame is opaque, so no blending.
        SDL_UpdateTexture(texture_, nullptr, frame.data(), static_cast<int>(frame.pitch()));
        SDL_RenderClear(renderer_);
        SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
        SDL_RenderPresent(renderer_);
    }

    void setFullscreen(bool fullscreen) override {
        if (window_ != nullptr) {
            SDL_SetWindowFullscreen(window_, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
        }
    }

    bool fullscreen() const override {
        return window_ != nullptr && (SDL_GetWindowFlags(window_) & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0;
    }

    int refreshRate() const override {
        SDL_DisplayMode mode;
        if (window_ != nullptr && SDL_GetWindowDisplayMode(window_, &mode) == 0) {
            return mode.refresh_rate;
        }
        return 0;
    }

    std::string describe() const override {
        if (renderer_ == nullptr) {
            return {};
        }
        int w = 0;
        int h = 0;
        int pw = 0;
        int ph = 0;
        SDL_GetWindowSize(window_, &w, &h);
        SDL_GetRendererOutputSize(renderer_, &pw, &ph);
        SDL_RendererInfo info;
        const char* name = SDL_GetRendererInfo(renderer_, &info) == 0 ? info.name : "?";
        return "window " + std::to_string(w) + "x" + std::to_string(h) + ", drawn at " + std::to_string(pw) +
               "x" + std::to_string(ph) + " pixels, renderer " + name;
    }

    bool vsync() const override { return vsync_; }

    bool setVsync(bool on) override {
        if (renderer_ != nullptr && SDL_RenderSetVSync(renderer_, on ? 1 : 0) == 0) {
            vsync_ = on;
        }
        return vsync_ == on;
    }

    bool readBack(core::gfx::Pixmap& out) const {
        if (renderer_ == nullptr) {
            return false;
        }
        int w = 0;
        int h = 0;
        if (SDL_GetRendererOutputSize(renderer_, &w, &h) != 0) {
            return false;
        }
        out = core::gfx::Pixmap(w, h);
        return SDL_RenderReadPixels(renderer_, nullptr, SDL_PIXELFORMAT_ABGR8888, out.data(),
                                    static_cast<int>(out.pitch())) == 0;
    }

private:
    SDL_Window* window_;
    SDL_Renderer* renderer_;
    SDL_Texture* texture_;
    int width_;
    int height_;
    bool vsync_ = false;
};

class Sdl2Platform final : public Platform {
public:
    explicit Sdl2Platform(bool headless) : headless_(headless), system_(headless) {}
    ~Sdl2Platform() override { SDL_Quit(); }

    std::unique_ptr<Display> createDisplay(const DisplayOptions& options, std::string& error) override {
        const bool headless = headless_ || options.headless;
        SDL_Window* window =
            SDL_CreateWindow(options.title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                             options.width, options.height,
                             // Drawn at the screen's resolution (Retina, high-DPI
                             // Windows); the frame keeps its 4:3 shape when resized.
                             headless ? SDL_WINDOW_HIDDEN
                                      : SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
        if (window == nullptr) {
            error = SDL_GetError();
            return nullptr;
        }
        if (headless) {
            // Headless runs never present anything.
            return std::make_unique<Sdl2Display>(window, nullptr, nullptr, options.width, options.height);
        }
        Uint32 flags = SDL_RENDERER_ACCELERATED;
        if (options.vsync) {
            flags |= SDL_RENDERER_PRESENTVSYNC;
        }
        SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, flags);
        if (renderer == nullptr) {
            SDL_Log("Accelerated renderer unavailable (%s), using the software renderer", SDL_GetError());
            renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
        }
        if (renderer == nullptr) {
            error = SDL_GetError();
            SDL_DestroyWindow(window);
            return nullptr;
        }
        SDL_RenderSetLogicalSize(renderer, options.width, options.height);
        // Pixmap pixels are 0xAABBGGRR integers: SDL's ABGR8888 on any byte order.
        SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888,
                                                 SDL_TEXTUREACCESS_STREAMING, options.width, options.height);
        if (texture == nullptr) {
            error = SDL_GetError();
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            return nullptr;
        }
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE);
        auto display =
            std::make_unique<Sdl2Display>(window, renderer, texture, options.width, options.height);
        if (options.fullscreen) {
            display->setFullscreen(true);
        }
        return display;
    }

    Clock& clock() override { return clock_; }
    InputSource& input() override { return input_; }
    Audio& audio() override { return audio_; }
    System& system() override { return system_; }

private:
    bool headless_;
    Sdl2Clock clock_;
    Sdl2System system_;
    Sdl2Input input_;
    Sdl2Audio audio_;
};

}  // namespace

bool readBack(Display& display, core::gfx::Pixmap& out) {
    // Every Display this backend creates is an Sdl2Display.
    return static_cast<const Sdl2Display&>(display).readBack(out);
}

std::unique_ptr<Platform> createPlatform(const Options& options, std::string& error) {
    if (options.headless) {
        SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
        SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);
    }
#if defined(SDL_HINT_WINDOWS_DPI_AWARENESS) && defined(SDL_HINT_WINDOWS_DPI_SCALING)
    // Windows: draw at the screen's resolution, with window sizes in points
    // as on macOS (SDL 2.24 and later). Not yet checked on Windows.
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_SCALING, "1");
#endif
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS |
                 SDL_INIT_TIMER) != 0) {
        error = SDL_GetError();
        return nullptr;
    }
    return std::make_unique<Sdl2Platform>(options.headless);
}

}  // namespace bb::platform::sdl2
