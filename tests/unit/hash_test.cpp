#include <doctest.h>

#include <cstdint>
#include <cstring>

#include "bb/core/hash.hpp"

TEST_CASE("Fnv1a64 matches the reference test vectors") {
    bb::core::Fnv1a64 empty;
    CHECK(empty.value() == 0xcbf29ce484222325ull);

    bb::core::Fnv1a64 a;
    a.addBytes("a", 1);
    CHECK(a.value() == 0xaf63dc4c8601ec8cull);

    bb::core::Fnv1a64 foobar;
    foobar.addBytes("foobar", std::strlen("foobar"));
    CHECK(foobar.value() == 0x85944171f73967e8ull);
}

TEST_CASE("Fnv1a64 hashes integers as little-endian bytes") {
    bb::core::Fnv1a64 viaInt;
    viaInt.add(std::uint32_t{0x04030201u});

    const unsigned char bytes[] = {1, 2, 3, 4};
    bb::core::Fnv1a64 viaBytes;
    viaBytes.addBytes(bytes, sizeof(bytes));

    CHECK(viaInt.value() == viaBytes.value());

    bb::core::Fnv1a64 negative;
    negative.add(-1);
    const unsigned char ff[] = {0xFF, 0xFF, 0xFF, 0xFF};
    bb::core::Fnv1a64 ffBytes;
    ffBytes.addBytes(ff, sizeof(ff));
    CHECK(negative.value() == ffBytes.value());
}

TEST_CASE("WordHash64 depends on every word and on their order") {
    bb::core::WordHash64 a;
    a.add(1u, 2u);
    a.add(3u, 4u);
    bb::core::WordHash64 b;
    b.add(1u, 2u);
    b.add(3u, 4u);
    CHECK(a.value() == b.value());

    bb::core::WordHash64 swapped;
    swapped.add(3u, 4u);
    swapped.add(1u, 2u);
    CHECK(a.value() != swapped.value());

    bb::core::WordHash64 oneBit;
    oneBit.add(1u, 2u);
    oneBit.add(3u, 5u);
    CHECK(a.value() != oneBit.value());

    bb::core::WordHash64 empty;
    bb::core::WordHash64 zeros;
    zeros.add(0u, 0u);
    CHECK(empty.value() != zeros.value());
}
