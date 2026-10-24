#include <doctest.h>

#include "bb/core/rng.hpp"

TEST_CASE("Rng reproduces the Microsoft CRT rand() sequence") {
    // First values of rand() in a Visual C++ program that never calls
    // srand(), i.e. the generator the 2002 game ran on.
    bb::core::Rng rng;
    const int expected[] = {41, 18467, 6334, 26500, 19169, 15724, 11478, 29358, 26962, 24464};
    for (int value : expected) {
        CHECK(rng.next() == value);
    }
}

TEST_CASE("Rng stays within [0, kMax] and reseeds") {
    bb::core::Rng rng(12345);
    for (int i = 0; i < 100000; ++i) {
        const int v = rng.next();
        REQUIRE(v >= 0);
        REQUIRE(v <= bb::core::Rng::kMax);
    }
    rng.seed(1);
    CHECK(rng.next() == 41);
    CHECK(rng.state() != 1u);
}
