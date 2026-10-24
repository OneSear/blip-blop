#include <doctest.h>

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include "bb/core/input/bindings.hpp"
#include "bb/core/input/names.hpp"
#include "bb/core/settings.hpp"
#include "bb/game/input.hpp"

namespace in = bb::core::input;

TEST_CASE("input codes have stable machine names") {
    CHECK(in::codeName(in::kKeyLeftCtrl) == "key:left_ctrl");
    CHECK(in::codeName(in::kKeyA) == "key:a");
    CHECK(in::codeName(in::kKeyF12) == "key:f12");
    CHECK(in::codeName(in::kKeyKeypad1) == "key:keypad_1");
    CHECK(in::codeName(in::padCode(0, 3)) == "pad1:button3");
    CHECK(in::codeName(in::padCode(1, in::kPadUp)) == "pad2:up");
    CHECK(in::codeName(0x10u) == "code:0x10");

    for (const char* name :
         {"key:left_ctrl", "key:a", "key:keypad_enter", "pad1:button3", "pad2:right", "code:0x10"}) {
        const auto code = in::codeFromName(name);
        REQUIRE_MESSAGE(code.has_value(), name);
        CHECK(in::codeName(*code) == name);
    }
    CHECK_FALSE(in::codeFromName("key:nope").has_value());
    CHECK_FALSE(in::codeFromName("pad0:button1").has_value());
    CHECK_FALSE(in::codeFromName("pad1:button128").has_value());
    CHECK_FALSE(in::codeFromName("mouse:left").has_value());

    CHECK(in::isKnownCode(in::kKeySpace));
    CHECK(in::isKnownCode(in::padCode(4, 127)));
    CHECK_FALSE(in::isKnownCode(0x10u));  // a DirectInput scan code (DIK_Q)
}

TEST_CASE("display names are the ones the 2002 game printed") {
    CHECK(in::displayName(in::kKeyLeftCtrl, in::Language::English) == "LEFT CONTROL");
    CHECK(in::displayName(in::kKeyUp, in::Language::English) == "UP ARROW");
    CHECK(in::displayName(in::kKeyA, in::Language::English) == "A");
    CHECK(in::displayName(in::kKeyA, in::Language::French) == "Q");  // AZERTY position table
    CHECK(in::displayName(in::padCode(0, 2), in::Language::English) == "JOY1 BUTTON2");
    CHECK(in::displayName(in::padCode(0, 2), in::Language::French) == "JOY1 BUTTON3");
    CHECK(in::displayName(in::padCode(1, in::kPadLeft), in::Language::English) == "JOY2 LEFT");
    CHECK(in::displayName(in::kKeyMenu, in::Language::English) == "UNDEFINED");
}

TEST_CASE("settings round-trip through their text form") {
    bb::core::Settings s;
    s.vsync = false;
    s.fullscreen = true;
    s.language = in::Language::French;
    s.bindings.set(1, in::Action::Fire, in::padCode(0, 5));
    const std::string text = s.serialize();
    CHECK(text.rfind("blipblop-settings 1\nvsync off\nfullscreen on\nlanguage french\np1.up key:up\n", 0) ==
          0);

    std::string error;
    std::vector<std::string> warnings;
    const auto back = bb::core::Settings::parse(text + "future.option 3\n", error, warnings);
    REQUIRE_MESSAGE(back.has_value(), error);
    CHECK(back->vsync == false);
    CHECK(back->fullscreen == true);
    CHECK(back->language == in::Language::French);
    CHECK(back->bindings == s.bindings);
    CHECK(warnings.size() == 1);

    CHECK_FALSE(bb::core::Settings::parse("vsync on\n", error, warnings).has_value());
    CHECK_FALSE(
        bb::core::Settings::parse("blipblop-settings 1\np1.fire key:nope\n", error, warnings).has_value());
}

TEST_CASE("the shipped bb.cfg imports with player 2 reset to the defaults") {
    // data/bb.cfg as shipped in data.zip (61 bytes).
    const std::uint32_t codes[] = {0x40000052, 0x40000051, 0x40000050, 0x4000004F, 0x78, 0x20, 0x63,
                                   0x10,       0x1F,       0x20,       0x21,       0x0F, 0x22, 0x23};
    std::vector<std::uint8_t> bytes = {1, 1, 0, 0, 0};
    for (const auto c : codes) {
        for (int i = 0; i < 4; ++i) {
            bytes.push_back(static_cast<std::uint8_t>((c >> (8 * i)) & 0xFF));
        }
    }
    REQUIRE(bytes.size() == 61);

    std::vector<std::string> notes;
    const auto s = bb::core::Settings::importLegacyCfg(bytes, notes);
    REQUIRE(s.has_value());
    CHECK(s->vsync);
    CHECK(s->bindings.get(0, in::Action::Up) == in::kKeyUp);
    CHECK(s->bindings.get(0, in::Action::Fire) == in::kKeyX);
    CHECK(s->bindings.get(0, in::Action::Jump) == in::kKeySpace);
    CHECK(s->bindings.get(0, in::Action::Special) == in::kKeyC);
    const auto defaults = in::Bindings::defaults();
    for (const auto a : in::kActions) {
        CHECK(s->bindings.get(1, a) == defaults.get(1, a));
    }
    CHECK(notes.size() == 1);

    CHECK_FALSE(bb::core::Settings::importLegacyCfg({1, 2, 3}, notes).has_value());
}

TEST_CASE("every displayed input name fits the 20-byte buffers of the legacy menus") {
    // Game::getName() copies names into char tmp[20]; names are UTF-8.
    std::size_t longest = 0;
    std::size_t longestCharacters = 0;
    const auto check = [&](bb::core::input::Code code) {
        if (!bb::core::input::isKnownCode(code)) {
            return;
        }
        for (const auto language : {bb::core::input::Language::English, bb::core::input::Language::French}) {
            const std::string name = bb::core::input::displayName(code, language);
            longest = std::max(longest, name.size());
            std::size_t characters = 0;
            for (const char c : name) {
                characters += (static_cast<unsigned char>(c) & 0xC0u) != 0x80u ? 1 : 0;
            }
            longestCharacters = std::max(longestCharacters, characters);
        }
    };
    for (bb::core::input::Code k = 0; k < 0x100; ++k) {
        check(k);
    }
    for (bb::core::input::Code k = 0x40000000u; k < 0x40001000u; ++k) {
        check(k);
    }
    for (int slot = 0; slot < 8; ++slot) {
        for (bb::core::input::Code b = 0; b < 0x400; ++b) {
            check(bb::core::input::padCode(slot, b));
        }
    }
    MESSAGE("longest input name: " << longestCharacters << " characters, " << longest << " bytes");
    CHECK(longest < 20);
}

TEST_CASE("each player gets a default pad layout until one of its actions is bound to a pad") {
    CHECK(in::Bindings::defaultPadCode(0, in::Action::Left) == in::padCode(0, in::kPadLeft));
    CHECK(in::Bindings::defaultPadCode(0, in::Action::Jump) == in::padCode(0, in::kPadButtonA));
    CHECK(in::Bindings::defaultPadCode(0, in::Action::Fire) == in::padCode(0, in::kPadButtonX));
    CHECK(in::Bindings::defaultPadCode(1, in::Action::Special) == in::padCode(1, in::kPadButtonB));

    in::Bindings b = in::Bindings::defaults();
    CHECK_FALSE(b.usesPad(0));
    CHECK_FALSE(b.usesPad(1));
    b.set(1, in::Action::Fire, in::padCode(2, 5));
    CHECK_FALSE(b.usesPad(0));
    CHECK(b.usesPad(1));
}

namespace {

// Holds a fixed set of codes, with the 2002 default aliases.
class FakeInput final : public bb::game::InputState {
public:
    std::vector<unsigned int> pressed;
    unsigned int aliases[20] = {};

    FakeInput() {
        const in::Bindings b = in::Bindings::defaults();
        for (int a = 0; a < in::kActionCount; ++a) {
            aliases[a] = b.get(0, in::kActions[static_cast<std::size_t>(a)]);
            aliases[10 + a] = b.get(1, in::kActions[static_cast<std::size_t>(a)]);
        }
    }
    bool held(unsigned int code) const override {
        return std::find(pressed.begin(), pressed.end(), code) != pressed.end();
    }
    unsigned int alias(int a) const override { return aliases[a]; }
};

}  // namespace

TEST_CASE("aliases accept the bound key and the default pad layout") {
    using bb::game::AliasControls;
    FakeInput input;
    const AliasControls p1(input, AliasControls::kFirstPlayer);
    const AliasControls p2(input, AliasControls::kSecondPlayer);

    input.pressed = {in::kKeyLeftCtrl};
    CHECK(p1.fire());
    CHECK_FALSE(p2.fire());

    // Pad 1 plays the first player, pad 2 the second.
    input.pressed = {in::padCode(0, in::kPadButtonA), in::padCode(1, in::kPadLeft)};
    CHECK(p1.jump());
    CHECK_FALSE(p1.fire());
    CHECK_FALSE(p1.left());
    CHECK(p2.left());
    CHECK_FALSE(p2.jump());

    // Binding a pad input turns the player's default layout off.
    input.aliases[AliasControls::kFirstPlayer + AliasControls::kFire] = in::padCode(0, in::kPadButtonA);
    CHECK(p1.fire());
    CHECK_FALSE(p1.jump());
    CHECK(p2.left());

    // Aliases outside the players' never use the pads.
    CHECK_FALSE(input.aliasHeld(7));
}
