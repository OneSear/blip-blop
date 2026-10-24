#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "bb/game/media.hpp"

namespace bb::platform {
class Audio;
}

namespace bb::core::assets {
struct SoundBank;
}

namespace bb::game {

// The audio output of the game and its two switches (sound_on, music_on of
// the 2002 code): the app points it at the platform's audio, and clears it
// at shutdown.
struct AudioOutput {
    platform::Audio* device = nullptr;
    bool soundOn = true;
    bool musicOn = true;
};

// A bank of sounds (SoundBank, SoundBankBB, Sound): every play of a
// looping sound must be stopped before it stops; nothing plays while the
// sound is off.
class SoundBank : public SoundPlayer {
public:
    explicit SoundBank(AudioOutput& output) noexcept : output_(output) {}
    ~SoundBank() override;
    SoundBank(const SoundBank&) = delete;
    SoundBank& operator=(const SoundBank&) = delete;

    // Loads the sounds of bank into the output (replacing the previous
    // ones); a sound the output cannot decode stays silent.
    void assign(const core::assets::SoundBank& bank);
    void clear();
    bool loaded() const noexcept { return !sounds_.empty(); }
    int size() const noexcept { return static_cast<int>(sounds_.size()); }
    AudioOutput& output() const noexcept { return output_; }

    // SoundPlayer. A sound past the end of the bank is ignored.
    void play(int sound, int flags = 0) override;
    void stop(int sound) override;

private:
    struct Sound {
        int sample = -1;
        int voice = -1;
        int loops = 0;  // looping plays not stopped yet
    };

    AudioOutput& output_;
    std::vector<Sound> sounds_;
};

// A bank of musics (MusicBank): one music plays at a time; stopping any
// music of any bank stops whichever music is playing (2014 port behaviour,
// kept). Nothing happens while the music is off.
class MusicBank : public MusicPlayer {
public:
    explicit MusicBank(AudioOutput& output) noexcept : output_(output) {}
    ~MusicBank() override;
    MusicBank(const MusicBank&) = delete;
    MusicBank& operator=(const MusicBank&) = delete;

    // Opens the musics (complete Ogg Vorbis or MP3 files); a file the
    // output cannot stream stays silent. loop: the musics repeat.
    void assign(std::vector<std::vector<std::uint8_t>> files, bool loop);
    void clear();
    int size() const noexcept { return static_cast<int>(tracks_.size()); }
    AudioOutput& output() const noexcept { return output_; }

    // MusicPlayer. A music past the end of the bank is ignored.
    void play(int music) override;
    void stop(int music) override;
    // Stops the music of this bank that plays.
    void stopAll();

private:
    struct Track {
        int music = -1;
        bool playing = false;
    };

    AudioOutput& output_;
    std::vector<Track> tracks_;
    bool loop_ = true;
};

}  // namespace bb::game
