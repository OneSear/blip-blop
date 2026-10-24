#include <doctest.h>

#include <cstring>

#include "bb/game/hi_scores.hpp"

TEST_CASE("The default table runs from 100,000 down to 10,000") {
    bb::game::HiScores table;
    CHECK(table.score(0) == 100000);
    CHECK(table.score(9) == 10000);
    CHECK(std::strcmp(table.name(4), "LOADED STUDIO") == 0);
    CHECK(table.score(10) == 0);
    CHECK(std::strcmp(table.name(-1), "") == 0);
}

TEST_CASE("A score goes above the equal ones and pushes the last one out") {
    bb::game::HiScores table;
    CHECK(table.qualifies(10001));
    CHECK_FALSE(table.qualifies(10000));

    table.add(50000, "BLIP");
    CHECK(table.score(5) == 50000);
    CHECK(std::strcmp(table.name(5), "BLIP") == 0);
    CHECK(table.score(6) == 50000);
    CHECK(std::strcmp(table.name(6), "LOADED STUDIO") == 0);
    CHECK(table.score(9) == 20000);
}

TEST_CASE("The table survives its file") {
    bb::game::HiScores table;
    table.add(123456, "BLOP");
    bb::game::HiScores read;
    REQUIRE(read.fromFile(table.toFile()));
    CHECK(read.score(0) == 123456);
    CHECK(std::strcmp(read.name(0), "BLOP") == 0);
    CHECK(read.toFile() == table.toFile());

    std::vector<std::uint8_t> damaged = table.toFile();
    damaged[0] ^= 1;
    CHECK_FALSE(read.fromFile(damaged));
}
