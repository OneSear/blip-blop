#include "bb/platform/sdl2/sdl2_platform.hpp"

#include <SDL.h>

#include "bb/core/audio/mixer.hpp"

#if defined(_WIN32)
// clang-format off
#include <windows.h>
#include <psapi.h>
// clang-format on
#else
#include <sys/resource.h>
#endif

#include <filesystem>
#include <memory>
#include <string>
#include <system_error>
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

private:
    void show(Uint32 flags, const std::string& title, const std::string& message) const {
        if (!headless_ && SDL_ShowSimpleMessageBox(flags, title.c_str(), message.c_str(), nullptr) != 0) {
            SDL_Log("%s: %s (no message box: %s)", title.c_str(), message.c_str(), SDL_GetError());
        }
    }

    bool headless_;
};

// Files through SDL_RWops (UTF-8 paths on every system); SDL 2 cannot list
// directories, the C++ library does (from UTF-8 paths as well).
class Sdl2File final : public core::ReadableFile {
public:
    Sdl2File(SDL_RWops* rw, std::uint64_t size) noexcept : rw_(rw), size_(size) {}
    ~Sdl2File() override { SDL_RWclose(rw_); }
    Sdl2File(const Sdl2File&) = delete;
    Sdl2File& operator=(const Sdl2File&) = delete;

    std::uint64_t size() const override { return size_; }

    bool readAt(std::uint64_t offset, void* out, std::size_t count) override {
        if (count == 0) {
            return true;
        }
        if (offset > size_ || SDL_RWseek(rw_, static_cast<Sint64>(offset), RW_SEEK_SET) < 0) {
            return false;
        }
        return SDL_RWread(rw_, out, 1, count) == count;
    }

private:
    SDL_RWops* rw_;
    std::uint64_t size_;
};

class Sdl2FileSystem final : public FileSystem {
public:
    std::unique_ptr<core::ReadableFile> open(const std::string& path) const override {
        SDL_RWops* rw = SDL_RWFromFile(path.c_str(), "rb");
        if (rw == nullptr) {
            return nullptr;
        }
        const Sint64 size = SDL_RWsize(rw);
        if (size < 0) {
            SDL_RWclose(rw);
            return nullptr;
        }
        return std::make_unique<Sdl2File>(rw, static_cast<std::uint64_t>(size));
    }

    Kind kind(const std::string& path) const override {
        std::error_code ec;
        const std::filesystem::file_status status = std::filesystem::status(std::filesystem::u8path(path), ec);
        if (std::filesystem::is_regular_file(status)) {
            return Kind::File;
        }
        if (std::filesystem::is_directory(status)) {
            return Kind::Directory;
        }
        return Kind::Missing;  // nor the other kinds (devices, sockets)
    }

    std::vector<std::string> list(const std::string& directory) const override {
        std::vector<std::string> names;
        std::error_code ec;
        for (std::filesystem::directory_iterator it(std::filesystem::u8path(directory), ec), end;
             !ec && it != end; it.increment(ec)) {
            names.push_back(it->path().filename().u8string());
        }
        return names;
    }

    bool writeAll(const std::string& path, const void* data, std::size_t size) override {
        SDL_RWops* rw = SDL_RWFromFile(path.c_str(), "wb");
        if (rw == nullptr) {
            return false;
        }
        const bool written = size == 0 || SDL_RWwrite(rw, data, 1, size) == size;
        return SDL_RWclose(rw) == 0 && written;
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
    // The output the 2014 port asked SDL_mixer for: 44.1 kHz stereo 16-bit,
    // 1024 frames per callback. SDL converts it when the device differs.
    static constexpr int kRate = 44100;

    Sdl2Audio() {
        SDL_AudioSpec wanted{};
        wanted.freq = kRate;
        wanted.format = AUDIO_S16SYS;
        wanted.channels = 2;
        wanted.samples = 1024;
        wanted.callback = &Sdl2Audio::callback;
        wanted.userdata = this;
        SDL_AudioSpec obtained{};
        device_ = SDL_OpenAudioDevice(nullptr, 0, &wanted, &obtained, 0);
        if (device_ == 0) {
            SDL_Log("SDL_OpenAudioDevice: %s", SDL_GetError());
            return;
        }
        SDL_PauseAudioDevice(device_, 0);
    }

    ~Sdl2Audio() override {
        if (device_ != 0) {
            SDL_CloseAudioDevice(device_);  // no callback runs after this
        }
    }

    Sdl2Audio(const Sdl2Audio&) = delete;
    Sdl2Audio& operator=(const Sdl2Audio&) = delete;

    bool available() const override { return device_ != 0; }

    Sample loadSample(const std::uint8_t* data, std::size_t size) override {
        if (device_ == 0) {
            return -1;
        }
        // Converted outside the lock: the callback goes on meanwhile.
        std::vector<std::int16_t> samples;
        if (!core::audio::decodeWav(data, size, kRate, samples)) {
            SDL_Log("loadSample: not a PCM WAV file");
            return -1;
        }
        const Lock lock(device_);
        return mixer_.addSound(std::move(samples));
    }

    void freeSample(Sample sample) override {
        const Lock lock(device_);
        mixer_.freeSound(sample);
    }

    int play(Sample sample, bool loop) override {
        const Lock lock(device_);
        return mixer_.play(sample, loop);
    }

    void stop(int voice) override {
        const Lock lock(device_);
        mixer_.stop(voice);
    }

    Music openMusic(std::vector<std::uint8_t> file) override {
        if (device_ == 0) {
            return -1;
        }
        auto bytes = std::make_unique<std::vector<std::uint8_t>>(std::move(file));
        auto stream = core::audio::openMusicStream(*bytes);
        if (!stream) {
            SDL_Log("openMusic: neither Ogg Vorbis nor MP3");
            return -1;
        }
        const Lock lock(device_);
        return mixer_.addMusic(std::move(bytes), std::move(stream));
    }

    void freeMusic(Music music) override {
        const Lock lock(device_);
        mixer_.freeMusic(music);
    }

    void playMusic(Music music, bool loop) override {
        const Lock lock(device_);
        mixer_.playMusic(music, loop);
    }

    void stopMusic() override {
        const Lock lock(device_);
        mixer_.stopMusic();
    }

    bool voicePlaying(int voice) {
        const Lock lock(device_);
        return mixer_.playing(voice);
    }

private:
    // The audio callback and these calls never run at the same time.
    class Lock {
    public:
        explicit Lock(SDL_AudioDeviceID device) : device_(device) {
            if (device_ != 0) {
                SDL_LockAudioDevice(device_);
            }
        }
        ~Lock() {
            if (device_ != 0) {
                SDL_UnlockAudioDevice(device_);
            }
        }
        Lock(const Lock&) = delete;
        Lock& operator=(const Lock&) = delete;

    private:
        SDL_AudioDeviceID device_;
    };

    static void SDLCALL callback(void* self, Uint8* stream, int bytes) {
        static_cast<Sdl2Audio*>(self)->mixer_.mix(reinterpret_cast<std::int16_t*>(stream), bytes / 4);
    }

    SDL_AudioDeviceID device_ = 0;
    core::audio::Mixer mixer_{kRate};
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

    void setScaling(bool integer, bool smooth) override {
        if (renderer_ == nullptr) {
            return;
        }
        SDL_RenderSetIntegerScale(renderer_, integer ? SDL_TRUE : SDL_FALSE);
        SDL_SetTextureScaleMode(texture_, smooth ? SDL_ScaleModeLinear : SDL_ScaleModeNearest);
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
        SDL_ScaleMode mode = SDL_ScaleModeNearest;
        SDL_GetTextureScaleMode(texture_, &mode);
        return "window " + std::to_string(w) + "x" + std::to_string(h) + ", drawn at " + std::to_string(pw) +
               "x" + std::to_string(ph) + " pixels, renderer " + name + ", scaling " +
               (SDL_RenderGetIntegerScale(renderer_) == SDL_TRUE ? "integer" : "fit") + ", filter " +
               (mode == SDL_ScaleModeNearest ? "sharp" : "smooth");
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
        display->setScaling(options.integerScaling, options.smoothFilter);
        if (options.fullscreen) {
            display->setFullscreen(true);
        }
        return display;
    }

    Clock& clock() override { return clock_; }
    InputSource& input() override { return input_; }
    Audio& audio() override { return audio_; }
    FileSystem& files() override { return files_; }
    System& system() override { return system_; }

private:
    bool headless_;
    Sdl2Clock clock_;
    Sdl2System system_;
    Sdl2FileSystem files_;
    Sdl2Input input_;
    Sdl2Audio audio_;
};

}  // namespace

bool readBack(Display& display, core::gfx::Pixmap& out) {
    // Every Display this backend creates is an Sdl2Display.
    return static_cast<const Sdl2Display&>(display).readBack(out);
}

bool voicePlaying(Audio& audio, int voice) {
    // Every Audio this backend creates is an Sdl2Audio.
    return static_cast<Sdl2Audio&>(audio).voicePlaying(voice);
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
