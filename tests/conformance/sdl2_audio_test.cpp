// The SDL 2 audio backend on SDL's dummy driver (no sound card needed):
// voice count, looping and stopping, and that every sound and music the
// shipped game data lists decodes through the in-house mixer (Phase 9).

#include <doctest.h>

#include <SDL.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include "bb/core/audio/mixer.hpp"
#include "bb/platform/audio.hpp"
#include "bb/platform/sdl2/sdl2_platform.hpp"

namespace {

std::unique_ptr<bb::platform::Platform> makePlatform() {
    bb::platform::sdl2::Options options;
    options.headless = true;  // dummy video and audio drivers
    std::string error;
    auto platform = bb::platform::sdl2::createPlatform(options, error);
    REQUIRE_MESSAGE(platform != nullptr, error);
    return platform;
}

void put16(std::vector<std::uint8_t>& out, unsigned v) {
    out.push_back(static_cast<std::uint8_t>(v & 0xFF));
    out.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
}

void put32(std::vector<std::uint8_t>& out, unsigned v) {
    put16(out, v & 0xFFFF);
    put16(out, (v >> 16) & 0xFFFF);
}

void putTag(std::vector<std::uint8_t>& out, const char* tag) {
    out.insert(out.end(), tag, tag + 4);
}

// A 16-bit mono 22050 Hz PCM WAV file of `frames` samples.
std::vector<std::uint8_t> makeWav(unsigned frames) {
    std::vector<std::uint8_t> wav;
    const unsigned dataBytes = frames * 2;
    putTag(wav, "RIFF");
    put32(wav, 36 + dataBytes);
    putTag(wav, "WAVE");
    putTag(wav, "fmt ");
    put32(wav, 16);
    put16(wav, 1);      // PCM
    put16(wav, 1);      // mono
    put32(wav, 22050);  // rate
    put32(wav, 44100);  // bytes per second
    put16(wav, 2);      // block align
    put16(wav, 16);     // bits
    putTag(wav, "data");
    put32(wav, dataBytes);
    for (unsigned i = 0; i < frames; ++i) {
        put16(wav, (i * 997u) & 0xFFFF);
    }
    return wav;
}

bool readFile(const std::string& path, std::vector<std::uint8_t>& out) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return false;
    }
    out.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    return true;
}

std::int32_t readInt32(const std::vector<std::uint8_t>& bytes, std::size_t at) {
    std::uint32_t v = 0;
    for (int i = 3; i >= 0; --i) {
        v = (v << 8) | bytes[at + static_cast<std::size_t>(i)];
    }
    return static_cast<std::int32_t>(v);
}

std::vector<std::string> filesWithExtension(const std::string& dir, const std::string& extension) {
    std::vector<std::string> names;
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (entry.path().extension() == extension) {
            names.push_back(entry.path().filename().string());
        }
    }
    return names;
}

}  // namespace

TEST_CASE("audio voices: one-shot, looping, 32 voices, stop") {
    auto platform = makePlatform();
    bb::platform::Audio& audio = platform->audio();
    REQUIRE(audio.available());

    const std::vector<std::uint8_t> wav = makeWav(44100);  // 2 s
    const auto sample = audio.loadSample(wav.data(), wav.size());
    REQUIRE(sample >= 0);

    // The game mixed 32 FMOD channels; the mixer has as many voices.
    CHECK(bb::core::audio::Mixer::kVoices == 32);

    std::vector<int> voices;
    for (int i = 0; i < 32; ++i) {
        const int voice = audio.play(sample, i % 2 == 0);
        CHECK(voice >= 0);
        voices.push_back(voice);
    }
    CHECK(audio.play(sample, false) == -1);  // all voices busy
    for (const int voice : voices) {
        CHECK(bb::platform::sdl2::voicePlaying(audio, voice));
    }

    audio.stop(voices[0]);
    CHECK_FALSE(bb::platform::sdl2::voicePlaying(audio, voices[0]));
    CHECK(audio.play(sample, false) == voices[0]);  // the freed voice is reused

    // Freeing a sample halts the voices that play it.
    audio.freeSample(sample);
    for (const int voice : voices) {
        CHECK_FALSE(bb::platform::sdl2::voicePlaying(audio, voice));
    }
    CHECK(audio.play(sample, false) == -1);  // invalid handle
}

TEST_CASE("audio rejects bytes that are not a WAV file") {
    auto platform = makePlatform();
    bb::platform::Audio& audio = platform->audio();
    const std::uint8_t junk[64] = {1, 2, 3, 4};
    CHECK(audio.loadSample(junk, sizeof junk) == -1);
    CHECK(audio.loadSample(nullptr, 10) == -1);
    CHECK(audio.openMusic(std::vector<std::uint8_t>(junk, junk + sizeof junk)) == -1);
    CHECK(audio.openMusic({}) == -1);
}

TEST_CASE("every shipped sound effect and music decodes") {
    const std::string dataDir = BB_TEST_DATA_DIR;
    std::vector<std::uint8_t> probe;
    if (!readFile(dataDir + "/bb.sfx", probe)) {
        MESSAGE("game data not extracted in " << dataDir << ", skipped");
        return;
    }

    auto platform = makePlatform();
    bb::platform::Audio& audio = platform->audio();
    REQUIRE(audio.available());

    // .sfx: int32 count, then per sound int32 buffer count, int32 size and
    // the WAV bytes (SoundBank::loadSFX).
    int sounds = 0;
    const std::vector<std::string> banks = filesWithExtension(dataDir, ".sfx");
    CHECK(banks.size() == 9);
    for (const std::string& bank : banks) {
        std::vector<std::uint8_t> bytes;
        REQUIRE_MESSAGE(readFile(dataDir + "/" + bank, bytes), bank);
        REQUIRE(bytes.size() >= 4);
        const std::int32_t count = readInt32(bytes, 0);
        REQUIRE(count > 0);
        std::size_t at = 4;
        for (std::int32_t i = 0; i < count; ++i) {
            REQUIRE(at + 8 <= bytes.size());
            const std::int32_t size = readInt32(bytes, at + 4);
            at += 8;
            REQUIRE(size > 0);
            REQUIRE(at + static_cast<std::size_t>(size) <= bytes.size());
            const auto sample = audio.loadSample(bytes.data() + at, static_cast<std::size_t>(size));
            CHECK_MESSAGE(sample >= 0, bank << " sound " << i);
            audio.freeSample(sample);
            at += static_cast<std::size_t>(size);
            ++sounds;
        }
        CHECK_MESSAGE(at == bytes.size(), bank);
    }
    MESSAGE(sounds << " sound effects decoded");

    // .mbk: count, then "type path" pairs (MusicBank::open).
    int musics = 0;
    const std::vector<std::string> musicBanks = filesWithExtension(dataDir, ".mbk");
    CHECK(musicBanks.size() == 12);
    for (const std::string& bank : musicBanks) {
        std::ifstream in(dataDir + "/" + bank);
        REQUIRE_MESSAGE(in.is_open(), bank);
        int count = 0;
        in >> count;
        REQUIRE(count > 0);
        for (int i = 0; i < count; ++i) {
            int type = -1;
            std::string path;
            in >> type >> path;
            CHECK_MESSAGE(type == 1, bank);
            REQUIRE(path.rfind("data/", 0) == 0);
            std::ifstream file(dataDir + "/" + path.substr(5), std::ios::binary);
            const std::vector<std::uint8_t> bytes(std::istreambuf_iterator<char>(file), {});
            const auto music = audio.openMusic(bytes);
            CHECK_MESSAGE(music >= 0, bank << " " << path);

            // Decoded to the end: at least a second (the shortest, the drum
            // roll, is about 1.3 s), and nothing silent throughout.
            auto stream = bb::core::audio::openMusicStream(bytes);
            REQUIRE_MESSAGE(stream != nullptr, path);
            std::vector<std::int16_t> frames(std::size_t{4096} * 2);
            long long total = 0;
            int loudest = 0;
            for (int got = 0; (got = stream->read(frames.data(), 4096)) > 0;) {
                total += got;
                for (int s = 0; s < got * 2; ++s) {
                    loudest =
                        std::max(loudest, std::abs(static_cast<int>(frames[static_cast<std::size_t>(s)])));
                }
            }
            CHECK_MESSAGE(total >= stream->rate(),
                          path << ": " << total << " frames at " << stream->rate() << " Hz");
            CHECK_MESSAGE(loudest > 1000, path);
            audio.playMusic(music, true);
            audio.stopMusic();
            audio.freeMusic(music);
            ++musics;
        }
    }
    MESSAGE(musics << " music entries decoded");
}
