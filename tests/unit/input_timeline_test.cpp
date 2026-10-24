#include <doctest.h>

#include <optional>
#include <string>

#include "bb/core/input_timeline.hpp"

using bb::core::InputState;
using bb::core::InputTimeline;

namespace {

std::optional<std::uint32_t> names(std::string_view name) {
    if (name == "RIGHT") {
        return 0x4000004Fu;
    }
    if (name == "RETURN") {
        return 13u;
    }
    return std::nullopt;
}

std::string toName(std::uint32_t code) {
    if (code == 0x4000004Fu) {
        return "RIGHT";
    }
    if (code == 13u) {
        return "RETURN";
    }
    return std::to_string(code);
}

}  // namespace

TEST_CASE("InputTimeline parses a script") {
    std::string error;
    const auto t = InputTimeline::parse(
        "# comment\n"
        "bbreplay 1\n"
        "seed 7\n"
        "option /cheat\n"
        "@10 RIGHT 0x20   # trailing comment\n"
        "@20\n"
        "@30 RETURN quit\n"
        "end 40\n",
        names, error);
    REQUIRE_MESSAGE(t.has_value(), error);
    CHECK(t->seed == 7u);
    REQUIRE(t->options.size() == 1);
    CHECK(t->options[0] == "/cheat");
    REQUIRE(t->endFrame.has_value());
    CHECK(*t->endFrame == 40u);

    CHECK(t->at(0).codes.empty());
    CHECK(t->at(9).codes.empty());
    CHECK(t->at(10).isPressed(0x4000004Fu));
    CHECK(t->at(15).isPressed(0x20u));
    CHECK(t->at(20).codes.empty());
    CHECK_FALSE(t->at(25).quit);
    CHECK(t->at(30).quit);
    CHECK(t->at(1000).isPressed(13u));
    // Non-monotonic queries still work.
    CHECK(t->at(12).isPressed(0x4000004Fu));
}

TEST_CASE("InputTimeline reports errors with line numbers") {
    std::string error;
    CHECK_FALSE(InputTimeline::parse("@1 RIGHT\n", names, error).has_value());
    CHECK(error.find("line 1") != std::string::npos);

    CHECK_FALSE(InputTimeline::parse("bbreplay 1\n@5\n@5 RIGHT\n", names, error).has_value());
    CHECK(error.find("line 3") != std::string::npos);

    CHECK_FALSE(InputTimeline::parse("bbreplay 1\n@5 JUMP\n", names, error).has_value());
    CHECK(error.find("JUMP") != std::string::npos);

    CHECK_FALSE(InputTimeline::parse("bbreplay 2\n", names, error).has_value());
    CHECK_FALSE(InputTimeline::parse("", names, error).has_value());
}

TEST_CASE("InputTimeline records only changes and round-trips") {
    InputTimeline t;
    InputState none;
    InputState right;
    right.press(0x4000004Fu);

    t.record(0, none);   // nothing yet: not stored
    t.record(5, right);
    t.record(6, right);  // unchanged: not stored
    t.record(9, none);
    t.record(9, right);  // same-frame change back: entry dropped
    CHECK(t.changes().size() == 1);

    t.record(12, none);
    t.endFrame = 20;
    t.seed = 3;
    const std::string text = t.serialize(toName);
    CHECK(text == "bbreplay 1\nseed 3\n@5 RIGHT\n@12\nend 20\n");

    std::string error;
    const auto again = InputTimeline::parse(text, names, error);
    REQUIRE_MESSAGE(again.has_value(), error);
    CHECK(again->serialize(toName) == text);
}

TEST_CASE("InputTimeline keeps the focus state") {
    InputTimeline t;
    InputState away;
    away.unfocused = true;
    InputState awayRight = away;
    awayRight.press(0x4000004Fu);

    t.record(3, away);
    t.record(7, awayRight);
    t.record(9, InputState{});
    REQUIRE(t.changes().size() == 3);
    CHECK_FALSE(t.at(2).unfocused);
    CHECK(t.at(3).unfocused);
    CHECK(t.at(8).unfocused);
    CHECK_FALSE(t.at(9).unfocused);

    const std::string text = t.serialize(toName);
    CHECK(text == "bbreplay 1\nseed 1\n@3 unfocused\n@7 RIGHT unfocused\n@9\n");
    std::string error;
    const auto again = InputTimeline::parse(text, names, error);
    REQUIRE_MESSAGE(again.has_value(), error);
    CHECK(again->at(7).unfocused);
    CHECK(again->at(7).isPressed(0x4000004Fu));
    CHECK(again->serialize(toName) == text);
}

TEST_CASE("InputState keeps codes sorted and unique") {
    InputState s;
    s.press(30);
    s.press(10);
    s.press(20);
    s.press(10);
    CHECK(s.codes == std::vector<std::uint32_t>{10, 20, 30});
    CHECK(s.isPressed(20));
    CHECK_FALSE(s.isPressed(25));
}
