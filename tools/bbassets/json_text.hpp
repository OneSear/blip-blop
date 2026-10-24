#pragma once

// JSON text for the source assets: human-readable and diff-friendly. Small
// containers of scalars (an event, a platform run) stay on one line; larger
// ones get one element per line. Keys keep their insertion order.

#include <string>

#include <nlohmann/json.hpp>

namespace bb::tools {

using Json = nlohmann::ordered_json;

std::string toJsonText(const Json& value);

}  // namespace bb::tools
