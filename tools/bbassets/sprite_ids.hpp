#pragma once

#include <map>
#include <string>

namespace bb::tools {

// The C++ header of every sprite bank whose manifest names images
// ("name": "weapon_smg") and of every sound bank whose manifest names
// sounds, by path relative to the include directory:
// "bb/assets/sprites/misc.hpp" declares bb::assets::sprites::misc::kWeaponSmg
// and kImageCount, "bb/assets/sounds/smurf.hpp" declares
// bb::assets::sounds::smurf::k... and kSoundCount. False and error when a
// name is invalid or used twice.
bool assetIdHeaders(const std::string& sourceDir, std::map<std::string, std::string>& headers, std::string& error);

}  // namespace bb::tools
