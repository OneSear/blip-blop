#pragma once

#include <string>
#include <vector>

#include "bb/game/media.hpp"

namespace bb::core::assets {
class AssetSource;
}

namespace bb::app {

// The strings of the game by id (uk.dat; txt_data.cpp of the legacy port).
class StringTable : public game::TextSource {
public:
    static constexpr int kCount = 2000;  // NB_TXT_DATA: 1000 in 2002; ids 1000+ hold the credits

    // Loads table `name` of the assets; false (logged) when it is missing.
    bool load(const core::assets::AssetSource& assets, const std::string& name);

    // game::TextSource: empty for an id without a string.
    const char* text(int id) const override;

private:
    std::vector<std::string> strings_;
    std::vector<bool> present_;
};

}  // namespace bb::app
