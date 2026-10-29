// The string table (txt_data.cpp of the legacy port).

#include "strings.hpp"

#include "bb/core/assets/asset_source.hpp"
#include "bb/core/log.hpp"

namespace bb::app {

bool StringTable::load(const core::assets::AssetSource& assets, const std::string& name) {
    std::string error;
    const auto table = assets.strings(name, error);
    if (!table) {
        core::Log::warning("loadTxtData() -> " + error);
        return false;
    }

    strings_.assign(kCount, std::string());
    present_.assign(kCount, false);
    // UTF-8, as the fonts read it.
    for (const auto& entry : table->strings) {
        if (entry.first < 0 || entry.first >= kCount) {
            core::Log::warning("loadTxtData() -> string id " + std::to_string(entry.first) + " out of range");
            continue;
        }
        strings_[static_cast<std::size_t>(entry.first)] = entry.second;
        present_[static_cast<std::size_t>(entry.first)] = true;
    }
    return true;
}

const char* StringTable::text(int id) const {
    if (id < 0 || id >= static_cast<int>(strings_.size()) || !present_[static_cast<std::size_t>(id)]) {
        return "";
    }
    return strings_[static_cast<std::size_t>(id)].c_str();
}

}  // namespace bb::app
