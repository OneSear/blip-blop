# The World and its entities

The game layer (`src/game`, library `bb_game`, standard C++ only) runs a
level on one `bb::game::World`. This page describes that framework as it
stands at the end of Phase 4 of [migration.md](../../migration.md), while
the 2002 classes and the ported ones run side by side.

## World

`World` (`bb/game/world.hpp`) owns everything a level runs on:

* **camera** (scroll position and speed, per-player scroll stop),
  **scroll lock**, **hold fire**, **weather**, **screen shake**, blink
  phases;
* **flags**: 11 values, 4 script registers (`Flag::User0`..`User3`) and
  values the game keeps up to date (`Kills`, `Enemies`, `Timer`, ...);
* **collision map**: per-pixel platform heights on 8 layers, solid and
  bloody wall grids, built from the level file;
* **media**: interfaces to the image banks, sound and music players, the
  canvas and the effects not yet ported (decals, blood sprays, weapon
  pickups) — `bb/game/media.hpp`;
* the session's **random generator** (one stream for the whole session, as
  in 2002: per-level generators would change every replay);
* the **target player** enemies aim at, and counters that carry over from
  level to level (target turn, treasure points, blood decal index);
* the **entity lists** and the **event lists**;
* the **entity registry** level events create entities from.

One `World`, owned by the application (`bb::app::App`, `src/app/app.cpp`),
is reused from level to level; `bb::game::Level` runs it tick by tick, and
the screens reach it through `bb::game::Services`.

## Entity lists

`EntityList<T>` keeps the 2002 `SuperListe` iteration rules, which the
replays depend on: iteration visits the newest entity first; one cursor
per list; an entity added during an iteration is visited by the next one;
removing the current entity moves to the next. A list owns its entities
(deletes them on removal and on `clear()`) or borrows them (players,
player shots, weather particles). Entities flagged `dead` are removed at
the end of the tick.

## Entity, roles and collisions

`Entity` (`entity.hpp`) is the base of every entity, old or ported: the
position, the hitbox (`x1`..`y2`, `collides`), `dead`, and the virtuals
`update()`, `draw()`, `state()` (the animation and character state the
world fingerprint records). The 2002 `Sprite` derives from it.

Old and new entities act on each other through pure **role** interfaces
with no data: `Shot` (damage, burning, blood direction, the 2002 direction
`heading()`, hit and kill callbacks), `ShotTarget` (hit by a shot, contact damage, counts for kills),
`Hurtable` (players), `Rider` (players, as vehicles see them) and
`Collectable` (pickups). An entity exposes its
roles with `asShot()` and the like. `World::resolveCollisions()` pairs the
lists in the 2002 order: player shots and cow bombs against enemies,
pickups against players, then enemies and enemy shots against players.

A reference from one entity to another is a `Handle<T>`: it becomes null
when the target is destroyed (D-04). A handle to a `dead` entity still
works until the end of the tick, as the raw pointers did.

## Base classes for ported content

`actor.hpp`: `Actor` (image, animation, gravity, walking on platforms),
`Character` (hit points and a state), `Enemy` (the shared death pipeline:
blood on walls, weapon pickups, kill counter; blood sprays; lobbed shots;
burning), `Projectile`. `gore.hpp`: `GoreDrop`, the generic gore drop
that explosions and hit bosses throw (also used by the 2002 cow bomb).
`vehicle.hpp`: `Vehicle`, boarded by a `Rider` walking into it.
`generators.hpp`: `EnemyGenerator`, `PickupGenerator` (both `Generator`s,
updated by the game every tick) and the pickup sets. Ported entities receive the `World&` at
construction and use `world()` instead of globals. States keep their 2002
numbers because the fingerprint records them.

## Entity registry

`EntityRegistry` (`entity_registry.hpp`) maps each placeable type —
enemy, prop or pickup, whose legacy ids overlap — to a factory, by legacy
id or by symbolic name (`<world>.<entity>`, e.g. `smurf.greedy`).
`spawn()` creates the entity at a `SpawnPoint`, adds it to its list (props
and moving platforms are distinct placements) and counts enemies for the
kill percentage. `legacy_registry.cpp` registers the 2002 classes; a
ported world registers its own (`registerSmurfWorld()`) in their place.

## Level events

`LevelEvent` (`level_event.hpp`) has an activation X, `wakes()` (the
camera reached it), `ready()` (its condition holds) and `run(World&)`.
Events wait in `sleepingEvents` in level order; `World::updateEvents()`
moves those the camera reached to `events`, then runs and destroys the
ready ones. `makeLevelEvent()` builds the ported event types from the
level file; the remaining 2002 types (music, banner text, generators,
vehicle) are `Event` subclasses behind the same interface.

## Determinism rules for ported code

The golden replays must stay identical when a class is ported:

* keep every random draw, in the same order (constructors included);
* keep the state numbers, the animation counters and every field the
  fingerprint reads (`EntityState`), including 2002 quirks such as a
  subclass member hiding `Sprite::dy`;
* fix a 2002 defect only in a separate change that re-records the
  fingerprints (Phase 5), and record it in migration.md Appendix D;
* check with `--trace-ticks` that the ported classes actually ran.

## Where things are

| What | Where |
|------|-------|
| World, collision map, flags | `src/game/include/bb/game/world.hpp`, `src/game/src/world.cpp` |
| Entity, roles, handles | `entity.hpp` |
| Lists | `entity_list.hpp` |
| Base classes | `actor.hpp`, `src/game/src/actor.cpp` |
| Registry | `entity_registry.hpp` |
| Level events | `level_event.hpp`, `src/game/src/level_events.cpp` |
| Media (banks, canvas, fonts, strings) | `media.hpp`; filled by `src/app/app.cpp` |
| The level's tick, drawing, fingerprint | `level.hpp`, `src/game/src/level.cpp` |
| Screens, campaign | `services.hpp`, `game.hpp`, `campaign.hpp`, `src/game/src/screens/` |
| Fixed-point trigonometry | `trig.hpp` |
| Smurf world | `src/game/src/smurf/` |
