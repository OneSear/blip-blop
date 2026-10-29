// The music decoders of the mixer (bb/core/audio/mixer.hpp): stb_vorbis for
// Ogg Vorbis and dr_mp3 for the two MP3 jingles (tambour, gameover). Built
// as its own library, without the strict warnings: the decoders are
// third-party code (third_party/README.md, migration.md DEC-11).

#include <cstring>
#include <memory>
#include <vector>

#include "bb/core/audio/mixer.hpp"

#define STB_VORBIS_NO_STDIO
#define STB_VORBIS_NO_PUSHDATA_API
#include "stb_vorbis.c"

#define DR_MP3_IMPLEMENTATION
#define DR_MP3_NO_STDIO
#include "dr_mp3.h"

namespace bb::core::audio {

namespace {

class VorbisStream final : public MusicStream {
public:
    explicit VorbisStream(stb_vorbis* vorbis) : vorbis_(vorbis), info_(stb_vorbis_get_info(vorbis)) {}
    ~VorbisStream() override { stb_vorbis_close(vorbis_); }
    VorbisStream(const VorbisStream&) = delete;
    VorbisStream& operator=(const VorbisStream&) = delete;

    int rate() const override { return static_cast<int>(info_.sample_rate); }

    int read(std::int16_t* out, int frames) override {
        // Mono files come out on both channels.
        return stb_vorbis_get_samples_short_interleaved(vorbis_, 2, out, frames * 2);
    }

    bool rewind() override { return stb_vorbis_seek_start(vorbis_) != 0; }

private:
    stb_vorbis* vorbis_;
    stb_vorbis_info info_;
};

class Mp3Stream final : public MusicStream {
public:
    Mp3Stream() = default;
    ~Mp3Stream() override {
        if (open_) {
            drmp3_uninit(&mp3_);
        }
    }
    Mp3Stream(const Mp3Stream&) = delete;
    Mp3Stream& operator=(const Mp3Stream&) = delete;

    bool open(const std::vector<std::uint8_t>& file) {
        open_ = drmp3_init_memory(&mp3_, file.data(), file.size(), nullptr) != 0;
        return open_ && (mp3_.channels == 1 || mp3_.channels == 2) && mp3_.sampleRate > 0;
    }

    int rate() const override { return static_cast<int>(mp3_.sampleRate); }

    int read(std::int16_t* out, int frames) override {
        const auto got =
            static_cast<int>(drmp3_read_pcm_frames_s16(&mp3_, static_cast<drmp3_uint64>(frames), out));
        if (mp3_.channels == 1) {
            // Spread in place, from the end: frame i goes to 2i and 2i+1.
            for (int i = got - 1; i >= 0; --i) {
                out[2 * i + 1] = out[i];
                out[2 * i] = out[i];
            }
        }
        return got;
    }

    bool rewind() override { return drmp3_seek_to_pcm_frame(&mp3_, 0) != 0; }

private:
    drmp3 mp3_{};
    bool open_ = false;
};

}  // namespace

std::unique_ptr<MusicStream> openMusicStream(const std::vector<std::uint8_t>& file) {
    if (file.size() < 4 || file.size() > 0x7FFFFFFFu) {
        return nullptr;
    }
    if (std::memcmp(file.data(), "OggS", 4) == 0) {
        int error = 0;
        stb_vorbis* vorbis =
            stb_vorbis_open_memory(file.data(), static_cast<int>(file.size()), &error, nullptr);
        if (vorbis == nullptr) {
            return nullptr;
        }
        auto stream = std::make_unique<VorbisStream>(vorbis);
        if (stream->rate() <= 0) {
            return nullptr;
        }
        return stream;
    }
    auto stream = std::make_unique<Mp3Stream>();
    if (!stream->open(file)) {
        return nullptr;
    }
    return stream;
}

}  // namespace bb::core::audio
