#pragma once

#include <array>
#include <optional>
#include <string>
#include <vector>

#include "bb/core/assets/legacy_formats.hpp"

namespace bb::game {

// The ten best scores and their names (the 2002 HiScores, saved as bb.scr).
class HiScores {
public:
    static constexpr int kCount = 10;
    static constexpr int kNameLength = 20;  // bytes, terminator included

    HiScores() { reset(); }

    // 100,000 down to 10,000, all "LOADED STUDIO" (init()).
    void reset();

    // Rank 0 is the best; past the table: an empty name and 0.
    const char* name(int rank) const;
    int score(int rank) const;

    // Whether score beats the last of the table (isGood()).
    bool qualifies(int score) const { return entries_[kCount - 1].score < score; }
    // Inserts score above the equal ones; the last one drops out.
    void add(int score, const char* name);

    // The bb.scr file; false when its size or checksum is wrong.
    bool fromFile(const std::vector<std::uint8_t>& bytes);
    std::vector<std::uint8_t> toFile() const;

private:
    struct Entry {
        std::array<char, kNameLength> name{};
        int score = 0;
    };
    std::array<Entry, kCount> entries_{};
};

}  // namespace bb::game
