# Cut content

Content of the 2002 game that no shipped level uses, archived here instead
of being ported (migration.md, Phases 5 and 6). The code is in the git
history (the commit before its deletion); this page says what it was, how
it was reached (or why it never was) and what it would take to bring back.

| Content | 2002 files | Why it is unused | Deleted |
|---------|------------|------------------|---------|
| Mario's hologram | `ennemi_mario_hologramme.cpp/.h` | Its only spawn site, in Mario's walk (`ennemi_mario.cpp`, below 12,500 HP), is commented out. | Batch 5h |
| The seahorse vehicle | `vehicule_hypo.cpp/.h` | Not in the vehicle factory (`event_vehicule.cpp` knows only the stork, id 0); no level places it. | Batch 5i |
| The test turret | `vehicule_test_tourelle.h` | A header no file includes. | Batch 5i |
| The heart bear (enemy 206) | `ennemi_bisou_coeur.cpp/.h`, `tir_coeur_bisou.cpp/.h`, `gore_tete_bisou_coeur.h` | No level places id 206. | Phase 6 (done during Phase 5) |
| The car bear | `ennemi_bisou_auto.cpp/.h`, `gore_bisou_auto.h`, `explosion.cpp/.h` | No id in the enemy factory; nothing creates it. | Phase 6 (done during Phase 5) |
| Diabolo (enemy 306) | `ennemi_diabolo.cpp/.h`, `tir_tornade.cpp/.h` | No level places id 306; unfinished. | Phase 6 (done during Phase 5) |
| Pac-Man | `ennemi_pacman.cpp/.h` | Its only spawn site, in the Pac-Man car (`ennemi_voiture_pacman.cpp:41–77`), is commented out. | Phase 6 (done during Phase 5) |
| The star (prop 705) | `fond_etoile.cpp/.h` | No level places prop 705. | Phase 6 (done during Phase 5) |
| The fish (props 302–305) | `fond_poissons.cpp/.h` | No level places them. | Phase 6 (done during Phase 5) |
| The airlocks (props 300, 301) | `fond_snork_sas.cpp/.h` | No level places them. | Phase 6 (done during Phase 5) |
| Test sprites | `fond_test.h`, `p_plan_test.h`, `impactlaser.h` (empty) | Headers no file includes. | Phase 6 (done during Phase 5) |

## Mario's hologram

A copy of the final boss that Mario was to summon below 12,500 HP, facing
the other way. 1 HP and immune to shots (its `estTouche()` is all
commented out), so it could only vanish by its own death code; it blinks
between the normal images and those 54 further in `finalbossenn` (4 ticks
every 50 to 100), walks, jumps with Mario's low jump gravity and throws
the same horizontal and vertical fireballs, with two of Mario's attacks.
Bringing it back means porting it on `bb::game::finalboss::Mario`'s
pieces (the fireballs are ported) and deciding how it should die.

## The seahorse vehicle

A seahorse (image 255 of the level's enemy bank) the players could ride
freely at 3 pixels per tick inside the screen, firing but not turning,
and leave with jump; once used it swam off to the right. It was never
added to the vehicle factory, so no level could create it. The vehicle
base it needs is ported (`bb::game::Vehicle`); it would also need the
rider's controls (the stork does not read them).

## The test turret

A turret vehicle (image 525 of the level's enemy bank) whose rider turned
it left and right in 24 steps; a header-only test, never compiled.

## The heart bear (enemy 206)

A Care Bear (300 HP) of `bisouenn` that fired wave-shaped heart shots;
killed, its head flew off as gore while the body sprinted on. Its burn
state was an empty `case ETAT_CARBONISE: break;`: a burnt heart bear froze
until it scrolled off. To bring it back: port it on `carebears::HalvedBear`'s
pieces, fix the burn, place it in a bisous level.

## The car bear

A homing flying car (800 HP) driven by a Care Bear, exploding into gore
parts and an explosion sprite (`explosion.cpp`, used by nothing else). It
had no enemy id, so no level could place it.

## Diabolo (enemy 306)

An unfinished Snork boss (30,000 HP): kicks, a tornado shot
(`tir_tornade.cpp`), a knee drop at fixed targets (x 590 and 50, with
`xmin = 80`), screen shakes, floating-point jump arithmetic
(`-8 + dx * 0.25`), `debug <<` prints and large commented-out sections
(D-42, D-51, D-56). It would need finishing, like the Big Smurf was.

## Pac-Man

Meant to come out of the Pac-Man car once it landed in videoboss (1000
HP, images from 223 of `videobossenn`): the car's steps that would have
spawned him (register 2 = 4 and 5) are commented out, so the car only
waits and ends the level. `videoboss::PacManCar::draw()` keeps the 2002
drawing of Pac-Man getting out, never reached.

## The star (prop 705)

A twinkling star of `finalbossfonda` (images 36 to 40, 16 steps every 3
ticks). No level placed it.

## The fish (props 302 to 305)

Four fish of the Snork props, red and purple, plain and camouflaged:
3-image swimming loops, gone 100 pixels off the screen. No level placed
them (the Snork levels use no props).

## The airlocks (props 300, 301)

A Snork set piece: two airlock doors (moving platforms) whose shared
static state opened them once the target player passed x 1400, writing
solid walls cell by cell as they moved and drawing straight to the back
buffer. No level placed them.

## Test sprites

`FondTest` and `PPlanTest` (`fond_test.h`, `p_plan_test.h`), a prop and a
foreground decal from the engine's tests, and an empty `impactlaser.h`.
