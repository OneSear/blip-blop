// The hi-score table (hi_scores.cpp).

#include "bb/game/hi_scores.hpp"

#include <cstring>

namespace bb::game {

namespace {

// Copies a C string into a name as strcpy did: up to and including the
// terminator, the bytes after it unchanged (bb.scr saves all 20 bytes, and
// its checksum counts them). The 2002 code did not check the length; a name
// is cut at 19 characters here.
void copyName(std::array<char, HiScores::kNameLength>& to, const char* from) {
    std::size_t i = 0;
    for (; i + 1 < to.size() && from[i] != '\0'; ++i) {
        to[i] = from[i];
    }
    to[i] = '\0';
}

}  // namespace

void HiScores::reset() {
    int value = 10000 * kCount;
    for (Entry& e : entries_) {
        e.score = value;
        copyName(e.name, "LOADED STUDIO");
        value -= 10000;
    }
}

const char* HiScores::name(int rank) const {
    return rank >= 0 && rank < kCount ? entries_[static_cast<std::size_t>(rank)].name.data() : "";
}

int HiScores::score(int rank) const {
    return rank >= 0 && rank < kCount ? entries_[static_cast<std::size_t>(rank)].score : 0;
}

void HiScores::add(int score, const char* newName) {
    int rank = 0;
    while (rank < kCount && entries_[static_cast<std::size_t>(rank)].score > score) {
        ++rank;
    }
    for (int i = kCount - 1; i > rank; --i) {
        Entry& to = entries_[static_cast<std::size_t>(i)];
        const Entry& from = entries_[static_cast<std::size_t>(i - 1)];
        to.score = from.score;
        copyName(to.name, from.name.data());
    }
    if (rank < kCount) {
        Entry& e = entries_[static_cast<std::size_t>(rank)];
        e.score = score;
        copyName(e.name, newName);
    }
}

bool HiScores::fromFile(const std::vector<std::uint8_t>& bytes) {
    std::string error;
    const auto table = core::assets::readScr(bytes, error);
    if (!table) {
        return false;
    }
    for (std::size_t i = 0; i < entries_.size(); ++i) {
        entries_[i].score = table->entries[i].score;
        std::memcpy(entries_[i].name.data(), table->entries[i].name.data(), kNameLength);
    }
    return true;
}

std::vector<std::uint8_t> HiScores::toFile() const {
    core::assets::HiScoreTable table;
    for (std::size_t i = 0; i < entries_.size(); ++i) {
        table.entries[i].score = entries_[i].score;
        std::memcpy(table.entries[i].name.data(), entries_[i].name.data(), kNameLength);
    }
    return core::assets::writeScr(table);
}

}  // namespace bb::game
