// Sound and music banks (sound.cpp, sound_bank.cpp, sound_bank_bb.cpp,
// music_bank.cpp).

#include "bb/game/audio.hpp"

#include "bb/core/assets/asset_types.hpp"
#include "bb/platform/audio.hpp"

namespace bb::game {

//-----------------------------------------------------------------------------
// SoundBank
//-----------------------------------------------------------------------------

SoundBank::~SoundBank() {
    clear();
}

void SoundBank::assign(const core::assets::SoundBank& bank) {
    clear();
    sounds_.resize(bank.sounds.size());
    for (std::size_t i = 0; i < bank.sounds.size(); ++i) {
        const std::vector<std::uint8_t>& wav = bank.sounds[i].wav;
        if (output_.device != nullptr && !wav.empty()) {
            sounds_[i].sample = output_.device->loadSample(wav.data(), wav.size());
        }
    }
}

void SoundBank::clear() {
    if (output_.device != nullptr) {
        for (const Sound& sound : sounds_) {
            if (sound.sample >= 0) {
                output_.device->freeSample(sound.sample);
            }
        }
    }
    sounds_.clear();
}

void SoundBank::play(int sound, int flags) {
    if (!output_.soundOn || sound < 0 || sound >= size()) {
        return;
    }
    Sound& s = sounds_[static_cast<std::size_t>(sound)];
    if (output_.device == nullptr || s.sample < 0) {
        return;
    }
    if ((flags & kLoop) != 0) {
        if (s.loops++ == 0) {
            s.voice = output_.device->play(s.sample, true);
        }
    } else {
        s.voice = output_.device->play(s.sample, false);
    }
}

void SoundBank::stop(int sound) {
    if (!output_.soundOn || sound < 0 || sound >= size()) {
        return;
    }
    Sound& s = sounds_[static_cast<std::size_t>(sound)];
    if (output_.device == nullptr || s.voice == -1) {
        return;
    }
    if (s.loops > 0) {
        if (--s.loops == 0) {
            output_.device->stop(s.voice);
            s.voice = -1;
        }
    } else {
        output_.device->stop(s.voice);
        s.voice = -1;
    }
}

//-----------------------------------------------------------------------------
// MusicBank
//-----------------------------------------------------------------------------

MusicBank::~MusicBank() {
    clear();
}

void MusicBank::assign(std::vector<std::vector<std::uint8_t>> files, bool loop) {
    clear();
    loop_ = loop;
    tracks_.resize(files.size());
    for (std::size_t i = 0; i < files.size(); ++i) {
        if (output_.device != nullptr) {
            tracks_[i].music = output_.device->openMusic(std::move(files[i]));
        }
    }
}

void MusicBank::clear() {
    if (output_.device != nullptr) {
        for (const Track& track : tracks_) {
            if (track.music >= 0) {
                output_.device->freeMusic(track.music);
            }
        }
    }
    tracks_.clear();
}

void MusicBank::play(int music) {
    if (!output_.musicOn || music < 0 || music >= size()) {
        return;
    }
    Track& t = tracks_[static_cast<std::size_t>(music)];
    if (output_.device != nullptr && t.music >= 0) {
        output_.device->playMusic(t.music, loop_);
    }
    t.playing = true;
}

void MusicBank::stop(int music) {
    if (!output_.musicOn || music < 0 || music >= size()) {
        return;
    }
    Track& t = tracks_[static_cast<std::size_t>(music)];
    if (!t.playing) {
        return;
    }
    if (output_.device != nullptr) {
        output_.device->stopMusic();
    }
    t.playing = false;
}

void MusicBank::stopAll() {
    for (int i = 0; i < size(); ++i) {
        if (tracks_[static_cast<std::size_t>(i)].playing) {
            stop(i);
        }
    }
}

}  // namespace bb::game
