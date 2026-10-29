#include "json_text.hpp"

namespace bb::tools {

namespace {

constexpr std::size_t kInlineWidth = 120;

bool isScalar(const Json& v) {
    return !v.is_array() && !v.is_object();
}

bool hasOnlyScalars(const Json& v) {
    for (const auto& item : v) {
        if (!isScalar(item)) {
            return false;
        }
    }
    return true;
}

// One-line form with spaces after ',' and ':'.
std::string inlineText(const Json& v) {
    if (isScalar(v)) {
        return v.dump();
    }
    std::string out = v.is_array() ? "[" : "{";
    bool first = true;
    for (auto it = v.begin(); it != v.end(); ++it) {
        if (!first) {
            out += ", ";
        }
        first = false;
        if (v.is_object()) {
            out += Json(it.key()).dump() + ": ";
        }
        out += inlineText(*it);
    }
    out += v.is_array() ? "]" : "}";
    return out;
}

// A scalar or an array of at most four scalars (a point, a run).
bool isSmall(const Json& v) {
    return isScalar(v) || (v.is_array() && v.size() <= 4 && hasOnlyScalars(v));
}

bool allSmall(const Json& v) {
    for (const auto& item : v) {
        if (!isSmall(item)) {
            return false;
        }
    }
    return true;
}

bool fitsInline(const Json& v) {
    return isScalar(v) || v.empty() || (allSmall(v) && inlineText(v).size() <= kInlineWidth);
}

void write(const Json& v, int indent, std::string& out) {
    if (fitsInline(v)) {
        out += inlineText(v);
        return;
    }
    const std::string pad(static_cast<std::size_t>(indent + 2), ' ');
    if (v.is_array() && allSmall(v)) {
        // Long lists of numbers or runs: several per line.
        out += "[\n" + pad;
        std::size_t column = pad.size();
        bool first = true;
        for (const auto& item : v) {
            const std::string text = inlineText(item);
            if (!first) {
                if (column + 2 + text.size() > kInlineWidth) {
                    out += ",\n" + pad;
                    column = pad.size();
                } else {
                    out += ", ";
                    column += 2;
                }
            }
            first = false;
            out += text;
            column += text.size();
        }
        out += "\n" + std::string(static_cast<std::size_t>(indent), ' ') + "]";
        return;
    }
    out += v.is_array() ? "[\n" : "{\n";
    bool first = true;
    for (auto it = v.begin(); it != v.end(); ++it) {
        if (!first) {
            out += ",\n";
        }
        first = false;
        out += pad;
        if (v.is_object()) {
            out += Json(it.key()).dump() + ": ";
        }
        write(*it, indent + 2, out);
    }
    out += "\n" + std::string(static_cast<std::size_t>(indent), ' ') + (v.is_array() ? "]" : "}");
}

}  // namespace

std::string toJsonText(const Json& value) {
    std::string out;
    write(value, 0, out);
    out += "\n";
    return out;
}

}  // namespace bb::tools
