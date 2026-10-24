#include <doctest.h>

#include <string>
#include <vector>

#include "bb/game/entity_list.hpp"

namespace {

using bb::game::EntityList;
using bb::game::Ownership;

struct Thing {
    Thing(int thingId, std::vector<int>& deletedIds) : id(thingId), deleted(deletedIds) {}
    ~Thing() { deleted.push_back(id); }
    int id;
    std::vector<int>& deleted;
};

std::vector<int> ids(EntityList<Thing>& list) {
    std::vector<int> out;
    for (list.rewind(); !list.atEnd(); list.next()) {
        out.push_back(list.current()->id);
    }
    return out;
}

}  // namespace

TEST_CASE("EntityList visits the newest entity first") {
    std::vector<int> deleted;
    EntityList<Thing> list(Ownership::Owning);
    for (int i = 1; i <= 3; ++i) {
        list.add(new Thing(i, deleted));
    }
    CHECK(list.size() == 3);
    CHECK(ids(list) == std::vector<int>{3, 2, 1});
    CHECK(list.current() == nullptr);

    std::vector<int> seen;
    list.forEach([&](const Thing* t) { seen.push_back(t->id); });
    CHECK(seen == std::vector<int>{3, 2, 1});
}

TEST_CASE("EntityList: an entity added during an iteration is seen by the next one") {
    std::vector<int> deleted;
    EntityList<Thing> list(Ownership::Owning);
    list.add(new Thing(1, deleted));
    list.add(new Thing(2, deleted));

    std::vector<int> seen;
    for (list.rewind(); !list.atEnd(); list.next()) {
        seen.push_back(list.current()->id);
        if (list.current()->id == 2) {
            list.add(new Thing(10, deleted));
        }
    }
    CHECK(seen == std::vector<int>{2, 1});
    CHECK(ids(list) == std::vector<int>{10, 2, 1});
}

TEST_CASE("EntityList: removal moves to the next entity, owning lists delete") {
    std::vector<int> deleted;
    EntityList<Thing> list(Ownership::Owning);
    for (int i = 1; i <= 5; ++i) {
        list.add(new Thing(i, deleted));
    }
    // The 2002 clean-up loop: remove even ids, otherwise advance.
    list.rewind();
    while (!list.atEnd()) {
        if (list.current()->id % 2 == 0) {
            list.removeCurrent();
        } else {
            list.next();
        }
    }
    CHECK(ids(list) == std::vector<int>{5, 3, 1});
    CHECK(deleted == std::vector<int>{4, 2});

    // Removing the head and the tail.
    list.rewind();
    list.removeCurrent();
    CHECK(list.current()->id == 3);
    list.next();
    list.removeCurrent();
    CHECK(list.atEnd());
    CHECK(ids(list) == std::vector<int>{3});

    list.clear();
    CHECK(list.empty());
    CHECK(deleted == std::vector<int>{4, 2, 5, 1, 3});
}

TEST_CASE("EntityList: borrowing lists and releaseCurrent never delete") {
    std::vector<int> deleted;
    Thing a(1, deleted);
    Thing b(2, deleted);
    {
        EntityList<Thing> list(Ownership::Borrowing);
        list.add(&a);
        list.add(&b);
        list.rewind();
        CHECK(list.releaseCurrent() == &b);
        CHECK(list.current() == &a);
        list.add(&b);
        list.clear();
        list.add(&a);
    }
    CHECK(deleted.empty());

    EntityList<Thing> owning(Ownership::Owning);
    Thing* c = new Thing(3, deleted);
    owning.add(c);
    owning.add(nullptr);
    CHECK(owning.size() == 1);
    owning.rewind();
    CHECK(owning.releaseCurrent() == c);
    CHECK(owning.releaseCurrent() == nullptr);
    CHECK(deleted.empty());
    delete c;
}
