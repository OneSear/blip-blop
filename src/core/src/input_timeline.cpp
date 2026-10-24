#include "bb/core/input_timeline.hpp"

#include <algorithm>
#include <cstdlib>
#include <sstream>

namespace bb::core {

void InputState::press(std::uint32_t code) {
    auto it = std::lower_bound(codes.begin(), codes.end(), code);
    if (it == codes.end() || *it != code) {
        codes.insert(it, code);
    }
}

bool InputState::isPressed(std::uint32_t code) const {
    return std::binary_search(codes.begin(), codes.end(), code);
}

namespace {

std::vector<std::string_view> splitWords(std::string_view line) {
    std::vector<std::string_view> words;
    std::size_t i = 0;
    while (i < line.size()) {
        while (i < line.size() && (line[i] == ' ' || line[i] == '\t' || line[i] == '\r')) {
            ++i;
        }
        std::size_t start = i;
        while (i < line.size() && line[i] != ' ' && line[i] != '\t' && line[i] != '\r') {
            ++i;
        }
        if (i > start) {
            words.push_back(line.substr(start, i - start));
        }
    }
    return words;
}

std::optional<std::uint64_t> parseNumber(std::string_view word) {
    if (word.empty()) {
        return std::nullopt;
    }
    std::string s(word);
    int base = 10;
    std::size_t offset = 0;
    if (s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        base = 16;
        offset = 2;
    }
    char* end = nullptr;
    const unsigned long long v = std::strtoull(s.c_str() + offset, &end, base);
    if (end == s.c_str() + offset || *end != '\0') {
        return std::nullopt;
    }
    return static_cast<std::uint64_t>(v);
}

}  // namespace

std::optional<InputTimeline> InputTimeline::parse(std::string_view text, const NameToCode& resolve,
                                                  std::string& error) {
    InputTimeline timeline;
    bool sawHeader = false;
    std::size_t lineNumber = 0;
    std::size_t pos = 0;

    auto fail = [&](const std::string& message) -> std::optional<InputTimeline> {
        error = "line " + std::to_string(lineNumber) + ": " + message;
        return std::nullopt;
    };

    while (pos <= text.size()) {
        std::size_t eol = text.find('\n', pos);
        if (eol == std::string_view::npos) {
            eol = text.size();
        }
        std::string_view line = text.substr(pos, eol - pos);
        pos = eol + 1;
        ++lineNumber;

        const std::size_t hashPos = line.find('#');
        if (hashPos != std::string_view::npos) {
            line = line.substr(0, hashPos);
        }
        const auto words = splitWords(line);
        if (words.empty()) {
            continue;
        }

        if (!sawHeader) {
            if (words.size() != 2 || words[0] != "bbreplay" || words[1] != "1") {
                return fail("expected 'bbreplay 1' header");
            }
            sawHeader = true;
            continue;
        }

        if (words[0] == "seed") {
            const auto v = words.size() == 2 ? parseNumber(words[1]) : std::nullopt;
            if (!v || *v > 0xFFFFFFFFull) {
                return fail("'seed' needs one 32-bit number");
            }
            timeline.seed = static_cast<std::uint32_t>(*v);
        } else if (words[0] == "option") {
            if (words.size() != 2) {
                return fail("'option' needs one word");
            }
            timeline.options.emplace_back(words[1]);
        } else if (words[0] == "end") {
            const auto v = words.size() == 2 ? parseNumber(words[1]) : std::nullopt;
            if (!v) {
                return fail("'end' needs a frame number");
            }
            timeline.endFrame = v;
        } else if (words[0].front() == '@') {
            const auto frame = parseNumber(words[0].substr(1));
            if (!frame) {
                return fail("bad frame number '" + std::string(words[0]) + "'");
            }
            if (!timeline.changes_.empty() && *frame <= timeline.changes_.back().frame) {
                return fail("frames must be strictly increasing");
            }
            InputState state;
            for (std::size_t i = 1; i < words.size(); ++i) {
                if (words[i] == "quit") {
                    state.quit = true;
                    continue;
                }
                if (words[i] == "unfocused") {
                    state.unfocused = true;
                    continue;
                }
                if (const auto number = parseNumber(words[i])) {
                    if (*number > 0xFFFFFFFFull) {
                        return fail("code out of range '" + std::string(words[i]) + "'");
                    }
                    state.press(static_cast<std::uint32_t>(*number));
                } else if (const auto code = resolve ? resolve(words[i]) : std::nullopt) {
                    state.press(*code);
                } else {
                    return fail("unknown input name '" + std::string(words[i]) + "'");
                }
            }
            timeline.changes_.push_back({*frame, std::move(state)});
        } else {
            return fail("unknown statement '" + std::string(words[0]) + "'");
        }
    }

    if (!sawHeader) {
        error = "empty file: expected 'bbreplay 1' header";
        return std::nullopt;
    }
    return timeline;
}

std::string InputTimeline::serialize(const CodeToName& name) const {
    std::ostringstream out;
    out << "bbreplay 1\n";
    out << "seed " << seed << "\n";
    for (const auto& option : options) {
        out << "option " << option << "\n";
    }
    for (const auto& change : changes_) {
        out << '@' << change.frame;
        for (const auto code : change.state.codes) {
            out << ' ' << (name ? name(code) : std::to_string(code));
        }
        if (change.state.quit) {
            out << " quit";
        }
        if (change.state.unfocused) {
            out << " unfocused";
        }
        out << '\n';
    }
    if (endFrame) {
        out << "end " << *endFrame << "\n";
    }
    return out.str();
}

const InputState& InputTimeline::at(std::uint64_t frame) const {
    static const InputState kNothing;
    if (changes_.empty() || frame < changes_.front().frame) {
        return kNothing;
    }
    if (cursor_ >= changes_.size() || changes_[cursor_].frame > frame) {
        cursor_ = 0;
    }
    while (cursor_ + 1 < changes_.size() && changes_[cursor_ + 1].frame <= frame) {
        ++cursor_;
    }
    return changes_[cursor_].state;
}

void InputTimeline::record(std::uint64_t frame, const InputState& state) {
    if (changes_.empty()) {
        if (state != InputState{}) {
            changes_.push_back({frame, state});
        }
        return;
    }
    Change& last = changes_.back();
    if (last.state == state) {
        return;
    }
    if (last.frame == frame) {
        last.state = state;
        // A same-frame change may have restored the state of the change
        // before it; drop the now redundant entry.
        if ((changes_.size() >= 2 && changes_[changes_.size() - 2].state == state) ||
            (changes_.size() == 1 && state == InputState{})) {
            changes_.pop_back();
        }
        return;
    }
    changes_.push_back({frame, state});
}

}  // namespace bb::core
