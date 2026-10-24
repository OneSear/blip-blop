#pragma once

#include <cstddef>
#include <vector>

namespace bb::game {

// Whether a list deletes the entities it drops.
enum class Ownership {
    // The list owns its entities: removeCurrent() and clear() delete them.
    Owning,
    // Someone else owns them (players, pooled shots, an entity also held
    // by an owning list): the list only forgets them.
    Borrowing,
};

// A typed list of entities with the iteration rules of the 2002 lists
// (SuperListe), which the simulation depends on: the order of updates,
// collisions and random draws must not change.
//
// - add() puts an entity at the head: iteration visits the newest entity
//   first.
// - Iteration uses one cursor per list (rewind(), atEnd(), current(),
//   next()). An entity added during an iteration is not visited by it (it
//   lands before the cursor), but the next iteration sees it.
// - removeCurrent() and releaseCurrent() drop the entity under the cursor
//   and move the cursor to the next one.
//
// The entities are stored oldest first, so adding is cheap and the cursor
// is an index that adding does not move.
template <class T>
class EntityList {
public:
    explicit EntityList(Ownership ownership) noexcept : ownership_(ownership) {}
    ~EntityList() { clear(); }

    EntityList(const EntityList&) = delete;
    EntityList& operator=(const EntityList&) = delete;

    Ownership ownership() const noexcept { return ownership_; }

    // Puts an entity at the head of the list (null is ignored).
    void add(T* entity) {
        if (entity != nullptr) {
            items_.push_back(entity);
        }
    }

    // Puts the cursor on the newest entity.
    void rewind() noexcept { cursor_ = static_cast<std::ptrdiff_t>(items_.size()) - 1; }

    bool atEnd() const noexcept { return cursor_ < 0; }

    // The entity under the cursor; null at the end.
    T* current() const noexcept { return atEnd() ? nullptr : items_[static_cast<std::size_t>(cursor_)]; }

    void next() noexcept {
        if (cursor_ >= 0) {
            --cursor_;
        }
    }

    // Drops the entity under the cursor (deleting it if the list owns it)
    // and moves to the next one.
    void removeCurrent() {
        T* entity = releaseCurrent();
        if (ownership_ == Ownership::Owning) {
            delete entity;
        }
    }

    // Drops the entity under the cursor without deleting it, moves to the
    // next one, and returns it (null at the end).
    T* releaseCurrent() {
        if (atEnd()) {
            return nullptr;
        }
        const auto index = static_cast<std::size_t>(cursor_);
        T* entity = items_[index];
        items_.erase(items_.begin() + static_cast<std::ptrdiff_t>(index));
        --cursor_;
        return entity;
    }

    // Empties the list, deleting the entities if the list owns them (the
    // newest first, the iteration order).
    void clear() {
        std::vector<T*> items;
        items.swap(items_);
        cursor_ = -1;
        if (ownership_ == Ownership::Owning) {
            for (auto it = items.rbegin(); it != items.rend(); ++it) {
                delete *it;
            }
        }
    }

    std::size_t size() const noexcept { return items_.size(); }
    // The size as an int, for the code that counts in int (the 2002
    // lists did): no unsigned arithmetic creeps into it.
    int count() const noexcept { return static_cast<int>(items_.size()); }
    bool empty() const noexcept { return items_.empty(); }

    // The entities, newest first, without touching the cursor.
    template <class F>
    void forEach(F&& f) const {
        for (auto it = items_.rbegin(); it != items_.rend(); ++it) {
            f(*it);
        }
    }

private:
    std::vector<T*> items_;  // oldest first
    std::ptrdiff_t cursor_ = -1;
    Ownership ownership_;
};

}  // namespace bb::game
