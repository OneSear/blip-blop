# Legacy map

Where each piece of the 2002 code went. Updated in the same commit as the
code that moves (migration.md, section 3.12). Paths on the left are relative
to `vc-projects/Blip_n_Blop_3/`. The French→English naming rules are in
Appendix B of [migration.md](../migration.md).

| Old file / symbol | New file / symbol | Phase | Notes |
|-------------------|-------------------|-------|-------|
| `rand()` (C library, 375 calls) | `bbRand()` → `bb::core::Rng` (`src/core/include/bb/core/rng.hpp`) | 0 | Same generator as the Microsoft CRT the game was built with; seed 1 |
| `l_timer.cpp` `LGetTime()`, `Engine/windows.cpp` `GetTickCount()`, `timeGetTime()` | unchanged, but read `session::nowMicroseconds()` in deterministic runs | 0 | Replaced by `FixedClock` in Phase 3 |
| — | `session.h/.cpp` (record, replay, fingerprints) | 0 | SDL glue; platform-free parts in `bb::core` (`InputTimeline`, `FrameClock`, `Fnv1a64`) |
| `blip_n_blop_3.cpp` `analyseCmdLine()` | `session::init()` | 0 | Legacy `/safe /cheat /quiet` still accepted |
| `Engine/ddraw.h/.cpp`, `Engine/dinput.h/.cpp` | deleted | 0 | Never used: every reference was in commented-out code |
| `STDAFX.H`, `BB3.*`, `MyDX.*`, `deps/SDL2-2.0.1` | deleted | 0 | MFC precompiled header, Visual Studio projects and caches, vendored SDL 2.0.1 binaries |
| `Blip'n Blop 3.log` | `docs/history/original-2001-run.log` | 0 | A log written by the original Windows build on 1 November 2001 |
| `CMakeLists.txt` (CMake 2.8) | `CMakeLists.txt` (root), `cmake/`, `vc-projects/Blip_n_Blop_3/CMakeLists.txt` | 0 | |
| `Engine/io.h/.cpp`, `path_fix.*` | `legacy_io.h/.cpp` (`bbOpen`, `bbRead`, `bbClose`, `bbFileSize`, `resolvePath`, `writablePath`) on `bb::core::Vfs` | 1 | `--data <dir>`; case and separators normalised at open time |
| `graphics.*`, `dd_gfx.*`, `sdl_surface.h`, `sdl_pixelformat.h`, `sdl_surfaceinfo.h`, `sdl_texture.h` | `surface.h/.cpp` (`Surface` on `bb::core::gfx::Pixmap`), `video.h/.cpp`, platform `Display` | 1 | Blits and fills checked against SDL in `tests/conformance` |
| `restore.*`, `checkRestore()`, `alpha_blend.*`, `Picture` BMP loaders, `loadPBK` | deleted | 1 | No DirectDraw surface loss any more; `alpha_blend` was dead code (D-58) |
| `LGXpacker::halfTone()` | `bb::core::gfx::darken()` | 1 | Darkens its rectangle only (D-32) |
| `LGXpacker::createLGX_0/1` (GDI encoders) | deleted | 1 | The GFX and FNT editors keep theirs in `winLGX.cpp` |
| SDL events in `input.cpp` | platform `InputSource` (`Sdl2Input`) | 1 | Stable pad slots, hot-plug |
| `key_translator.cpp` tables | `bb::core::input::displayName()` (`src/core/src/input_names_tables.inc`) | 1 | Machine names such as `key:left_ctrl` in the settings file |
| `config.cpp`, binary `data/bb.cfg` | `bb::core::Settings`, text `settings.cfg` in the preferences directory (`--settings`) | 1 | `bb.cfg` imported once, never written (D-67) |
| `fmod.h`, `fmod__errors.h`, FMOD part of `Engine/windows.cpp` | platform `Audio` (`Sdl2Audio`), called by `sound.cpp` and `music_bank.cpp` | 1 | 32 voices (D-72) |
| `fmod.dll`, `fmodvc.lib`, `ddraw.lib`, `dinput.lib` | deleted | 1 | Link inputs of the deleted Visual Studio projects |
| `Engine/windows.h/.cpp` | `legacy_types.h` (`RECT`, `DWORD`, `COLORREF`, `RGB`, `using namespace std`); the stubs are deleted | 1 | Legacy sources must not include the real `<windows.h>` |
| `l_timer.*` (`LInitTimer`, `LGetTime`) | `legacy_time.h` `LGetTime()` on `session::nowMicroseconds()` | 1 | Exact integer time (D-71); `FixedClock` in Phase 3 |
| `GetTickCount()`, `timeGetTime()`, `Sleep()` | `bbTickCount()`, `bbSleep()` (`legacy_time.h`), `session::sleepMilliseconds()` | 1 | Virtual time in deterministic runs, as before; `bbSleep()` and `sleepMilliseconds()` deleted in Phase 3 (`DelayScreen`) |
| `ben_divers.*` (`manageMsg`, `wait`, `active`, `want_to_kill_app`) | deleted; `app_killed` defined in `input.cpp` | 1 | `manageMsg()` was a no-op in the SDL port |
| `WinProc`, `Question()`, `SetTimer` frame counter, `winSet` | deleted; the F12 frame counter runs on game time in the `Game` loop | 1 | D-45 |
| `Bug()`, `Warning()` (`MessageBox`) | `bb::core::Log` plus platform `System::showError()` / `showWarning()` | 1 | |
| `Debug` (`ben_debug.*`) | adapter onto `bb::core::Log` (`FileLogSink`, `ConsoleLogSink`) | 1 | Buffered; `BlipBlop.log` in the preferences directory, or `--log` |
| `main()` in `blip_n_blop_3.cpp` | `bbMain()` in `src/app/main.cpp`, `main()` in `src/platform/sdl2/sdl2_main.cpp`, `runLegacyGame()` | 1 | The legacy code is the object library `bb_legacy`; it compiles without SDL headers |
| `HiScores::crypte()`, checksums | same file format, unsigned arithmetic | 1 | D-70 |
| `LGXpacker::loadLGX()` (decoding), `PictureBank::loadGFX()` | `bb::core::assets::readGfx()`, `decodeLgx()`, `toPixmap()` (`src/core/include/bb/core/assets/`) | 2 | `PictureBank` builds its pictures from the decoded bank |
| `Fonte::load()`, byte-indexed `pictab[256]` | `readLft()`; `Fonte` looks glyphs up by Unicode code point in UTF-8 text | 2 | A byte outside a UTF-8 sequence counts as its Latin-1 character |
| `SoundBank::loadSFX()` | `readSfx()` | 2 | |
| `MusicBank::open()` (`ifstream`, host paths) | `readMbk()`; `Audio::openMusic()` takes the file bytes | 2 | Music can come from a pack |
| `Game::chargeNiveau()` (`_read` of `FICEVENT`) | `readLvl()` → `LevelData`, `LevelEventRecord` | 2 | Same field-to-event mapping; the 2002 garbage values are kept |
| `Game::loadList()` | `readLst()` → `Campaign` | 2 | |
| `loadTxtData()` (`ifstream`) | `readDat()` → `StringTable`, UTF-8; `gameText(id)` | 2 | Table of 2000 strings; ids 840–849 and 1000–1098 are the former literals |
| `Game::showCredits()` and `Game::showPE()` literals | `uk.dat` ids 1000–1098 and 840–849 | 2 | Layout still in code until Phase 3 |
| `CINEPlayer` / `RPGPlayer` `ifstream` | `std::istringstream` over `AssetSource::script()`; `parseCutscene()`, `dialogueScene()` in core | 2 | The players still parse as in 2002 |
| `HiScores::load/save`, `crypte()` | `readScr()` / `writeScr()` | 2 | Checksum on signed chars on every platform (D-74) |
| `precache.h` (`Precache()`) | deleted | 2 | Opened the file and read nothing (it used `fseek`'s return value, 0, as the size) |
| `resolvePath()` (host paths of data files) | deleted; assets come from `assets()`, a `bb::core::assets::AssetSource` | 2 | `LegacyDataSource` (the `data/` files) or `PackSource` (`--pack`) |
| `restoreAll()` of `PictureBank` and `Fonte` | deleted | 2 | Lost-surface handling; never called since Phase 1 |
| `pbk_misc[8]` … `pbk_misc[49]` in the pickups and the HUD | `bb::assets::sprites::misc::kWeaponSmg` … `kHudCowBomb` (`src/assets/include/`, generated by `bbassets ids`) | 2 | First symbolic sprite names; the rest are named as the content migrates (Phase 5) |
| GFX, FNT, SFX editors' output, level editor's `.lvl` | `bbassets` (`tools/bbassets/`): `extract`, `pack`, `legacy`, `verify`, `ids` | 2 | The MFC editors stay usable: `bbassets legacy` writes their files |
| `Game::go()`, `showMainScreen()`, `MenuMain` loop | `AttractScreen` (`screens/attract_screen.*`), the root of the screen stack | 3 | One loop: `runScreens()` (`screens/screen_runner.*`) on `bb::game::ScreenStack` (`src/game/`) |
| `showHighScores()`, `showGameOver()`, `getName()`, title picture | `HiScoreTableScreen`, `GameOverScreen`, `HiScoreEntryScreen`, `TitleScreen` (`screens/attract_screens.*`) | 3 | |
| `showCredits()` | `CreditsScreen` (`screens/credits_screen.*`) | 3 | Own `FixedClock` |
| `selectPlayer()` | `CharacterSelectScreen` (`screens/character_select_screen.*`) | 3 | |
| `jouePartie()` | `CampaignScreen` (`screens/campaign_screen.*`); `Game::startCampaign()`, `finishCampaign()` | 3 | |
| `joueNiveau()`, `gameLoop()`, `updateAll()` | `LevelScreen` (`screens/level_screen.*`); `Game::startLevel()` … `finishLevelLoop()` | 3 | |
| `showPE()` | `LevelSummaryScreen` (`screens/level_summary_screen.*`) | 3 | Body kept verbatim; members still have the 2002 local names |
| `updateMenu()`, `MenuGame` loop | `PauseScreen` (`screens/pause_screen.*`); `MenuGame::update(int &)` resumable | 3 | Runs inside a tick, which resumes after it |
| `updateRPG()`, `RPGPlayer` loop | `DialogueScreen` (`screens/dialogue_screen.*`); `RPGPlayer::drawScene()`, `skip()` | 3 | Runs at the end of a tick |
| `CINEPlayer::playScene()` | `CutsceneScreen` (`screens/cutscene_screen.*`); `CINEPlayer::begin()`, `step()` | 3 | Own `FixedClock` |
| `Input::waitKey()`, `Input::waitClean()` | `WaitKeyScreen`, `WaitReleaseScreen`; `Input::firstKeyHeld()`, `anyKeyHeld()` | 3 | |
| "glorf" frame-time average, `Game` `time`/`dtime`/`tupdate`/`tdraw` | `bb::core::FixedClock` (`src/core/include/bb/core/fixed_clock.hpp`) | 3 | 11.8 ms ticks, at most 3 per frame |
| Literals "PLEASE WAIT", "LOADING", "Press a key to start.", "Loading...", "PLAYER %d GOT A HIGH SCORE!", "PLEASE ENTER YOUR NAME :", HUD "Bonus"/"Stage" | `uk.dat` ids 850–857, `TXT_PLEASE_WAIT` … `TXT_HUD_STAGE` (`txt_data.h`) | 3 | |
| — (focus events ignored) | `Input::takeFocusLost()`, `unfocused` in `.bbreplay` | 3 | A focus loss pauses the level |
| `Game::drawHUB()`, `drawHUBpv()`, `drawTimer()` | `Hud::draw()` (`screens/hud.*`) | 3 | Player 2 is player 1's layout mirrored |
| — (cutscenes ignored a closed window) | `CINEPlayer::step()` ends the scene when `app_killed` is set | 3 | Quit handled by every screen |
| `super_liste.*` (`SuperListe`) | `bb::game::EntityList<T>` (`src/game/include/bb/game/entity_list.hpp`) | 4 | Same iteration rules; owning lists `delete` (D-01) |
| `globals.h/.cpp` | `bb::game::World` (`world.hpp`); legacy aliases in `legacy_world.h/.cpp`, assets in `legacy_assets.h/.cpp` | 4 | The aliases go as classes are ported |
| `Sprite` fields `x`, `y`, `x1`..`y2`, `col_on`, `a_detruire` | `bb::game::Entity` (`entity.hpp`): `collides`, `dead` | 4 | `Sprite` derives from `Entity` |
| `Sprite::collision()` | `Entity::hits()` / `overlaps()` | 4 | The laser overrides `hits()` |
| `draw(x, y, pic)` (helper) | `drawPicture(x, y, pic)` | 4 | Renamed: the member `draw()` hid it |
| `Game::manageCollisions()` | `World::resolveCollisions()` | 4 | Roles `Shot`, `ShotTarget`, `Hurtable`, `Collectable` |
| `Sprite`, `Personnage`, `Ennemi`, `Tir` (for ported content) | `Actor`, `Character`, `Enemy`, `Projectile` (`actor.hpp`) | 4 | `tirEnCloche()` = `Enemy::lob()`, `onCarbonise()` = `Enemy::burn()` |
| `tete_turc`, `wait_for_bonus`, `num_giclure` | `World::target`, `treasure`, `bloodDecal` | 4 | `tete_turc` kept in step until its users are ported |
| `bSin`, `bCos` (`ben_maths`) for ported content | `sine1024()`, `cosine1024()` (`trig.hpp`) | 4 | Constant table, checked against libm |
| `event_ennemi.cpp`, `event_fond_anime.cpp`, `event_bonus.cpp` switches | `bb::game::EntityRegistry` (`entity_registry.hpp`); 2002 classes in `legacy_registry.cpp` | 4 | Symbolic names `<world>.<entity>` |
| `Event` (`aReveiller`, `aActiver`, `doEvent`), `Game::updateEvents()` | `bb::game::LevelEvent` (`wakes`, `ready`, `run`), `World::updateEvents()` (`level_event.hpp`) | 4 | `Event` bridges the 2002 events still in use |
| `EventEnnemi`/`EventFondAnime`/`EventBonus` (from level files), `EventLock`, `EventScrollSpeed`, `EventSetFlag`, `EventHoldFire`, `EventMeteo`, `EventSon`, `EventRPG`, `EventMiFond`, `EventPremierPlan` (+ `PPlan`) | `SpawnEvent`, `ScrollLockEvent`, `ScrollSpeedEvent`, `SetFlagEvent`, `HoldFireEvent`, `WeatherEvent`, `SoundEvent`, `DialogueEvent`, `DecalEvent` (`makeLevelEvent()`) | 4 | `EventEnnemi` and `EventBonus` remain as wrappers for direct callers |
| Raw cross-entity pointers (`princesse`, `lanceur`, `cible`, `tir_poing`, `Giclure::pere`) | `bb::game::Handle<T>` (`entity.hpp`) | 4 | D-04 |
| `EnnemiSmurf` + `EnnemiSmurfALunettes/Coquet/Frileux/Paresseux/Peon` | `bb::game::smurf::Smurf` with `Skin` (`src/game/src/smurf/smurf.cpp`) | 4 | Skins Plain, Brainy, Vain, Chilly, Lazy, Peon |
| `EnnemiSmurfGourmand`, `TirGateau` | `GreedySmurf`, `Cake` (`greedy_smurf.cpp`) | 4 | |
| `EnnemiSmurfFarceur`, `TirCadeau` | `PranksterSmurf`, `Gift` (`prankster_smurf.cpp`) | 4 | |
| `EnnemiSmurfSauvage`, `TirSmurfSauvage` | `WildSmurf`, `WildSmurfShot` (`wild_smurf.cpp`) | 4 | Shared timer kept (D-78) |
| `EnnemiSmurfPaysan` | `FarmerSmurf` (`farmer_smurf.cpp`) | 4 | |
| `EnnemiSmurfCostaud`, `TirHadoken` | `StrongmanSmurf`, `Fireball` (`strongman_smurf.cpp`) | 4 | D-76 kept |
| `EnnemiSmurfette`, `TirBisouMortel`, `EnnemiSmurfCigogne` | `Smurfette`, `DeadlyKiss`, `Stork` (`smurfette.cpp`) | 4 | D-77 kept |
| `EnnemiGrandSmurf` (id 8) | unchanged (legacy registry) | — | Never placed; Phase 5 decides (cut content) |
| `EnnemiPikachu`, `EnnemiBulbizarre`, `EnnemiFlameche` | `bb::game::pokemon::Walker` with `kPikachu`, `kBulbasaur`, `kCharmander` (`src/game/src/pokemon/enemies.cpp`) | 5b | One class, data per species |
| `EnnemiCOM`, `EnnemiCOMVolant` | `Com`, `FlyingCom` (`enemies.cpp`) | 5b | |
| `FondPokeAlarme1/2/3`, `FondPokeFleche`, `FondPokeCiel`, `FondPokeLevel2/3` | `Alarm`, `Arrow`, `SkyWindow`, `NeonSign` (`decor.cpp`) | 5b | |
| `FondPokeAscenceur`, `FondPokeAscenceur2`, `FondPokeInterupteur` | `Lift` (switches needed 0 or 1), `Switch` (`machines.cpp`) | 5b | `Lift` is a `MovingPlatform` |
| `FondPokeCaisseTombante`, `FondPokeCaissePlateforme`, `FondPokeCaissePlateforme2`, `FondPokeCaisse` | `LockedCrate`, `CratePlatform`, `SwingingPlatform`, `FallingCrate` (`machines.cpp`) | 5b | |
| `FondBombe`, `save_time` | `Bomb` (`machines.cpp`); the timer is held by the bomb defused last | 5b | D-50 |
| `y_plat[layer][i] = h` loops in moving platforms | `CollisionMap::setPlatform()`, `MovingPlatform::setPlatform()` | 5b | Clipped (D-41) |
| `EnnemiGrandSmurf` (id 8, unfinished, never placed) | `bb::game::smurf::BigSmurf`, `LobbedGift` (`big_smurf.cpp`); art `tools/art/big_smurf.py`; placed in snuf2 | 5a | Finished in 2026 |
| `EnnemiBisouCiel`, `EnnemiBisouEtoile` | `bb::game::carebears::SkyBear`, `StarBear` on `HalvedBear` (`src/game/src/carebears/bears.cpp`) | 5c | |
| `EnnemiBisouBoom`, `EnnemiBisouNuage` | `BoomBear`, `CloudBear` (`bears.cpp`) | 5c | |
| `EnnemiBisouZombi`, `EnnemiBisouChaman` | `ZombieBear`, `Shaman` (`zombie_shaman.cpp`) | 5c | |
| `EnnemiBisouJedi`, `EnnemiBisouJediHeros`, `EnnemiBisouSIB` | `Jedi`, `JediHero`, `SnorkCameo` (`jedi_cameo.cpp`) | 5c | |
| `TirArcCiel`, `TirEtoile`, `TirNuage`, `TirEpeeLaser`, `TirBouleChaman`, `TirColonneChaman` | `Rainbow`, `BouncingStar`, `StormCloud`, `Lightsaber`, `ShamanBall`, `ShamanColumn` (`shots.cpp`) | 5c | D-79, D-80 kept |
| `GorePiedsBisouCiel`, `GorePiedsBisouZombi`, `GorePiedsBisouEtoile`, `GorePied/Tete/BideBisouBoom` | `WalkingLegs`, `DancingLegs`, `BoomPiece` (`bears.cpp`) | 5c | |
| `EnnemiSnorkyBase1`, `EnnemiSnorkyBase2`, `EnnemiSnorkyGouverneur`, `EnnemiSnorkInBlack` | `bb::game::snorks::Grunt`, `Gunner`, `Governor`, `SnorkInBlack` on `Trooper` (`src/game/src/snorks/troopers.cpp`) | 5d | |
| `EnnemiSnorkyMage`, `EnnemiSnorkyHyporider` | `Mage`, `SeahorseRider` (`troopers.cpp`) | 5d | |
| `EnnemiSnorkInBlackBoss` | `SnorkBoss` (`boss_shots.cpp`) | 5d | Perch positions kept |
| `TirSnorkybase`, `TirSnorkyBase2`, `TirSnorkyGouverneur`, `TirSnorkyMage`, `TirEpine` (Snork uses) | `GruntShot`, `GunnerShot`, `GovernorShot`, `MageShot`, `Spine` (`boss_shots.cpp`) | 5d | `TirEpine` stays for Lara, Sonic |
| `MorceauSnorkyBaseTeteEntiere`, `...TetePartieArriere`, `...Trompe`, `MorceauSnorkyHyporidercorp` | `GruntHead`, `GruntHeadBack`, `GruntTrunk`, `RiderBody` (`boss_shots.cpp`) | 5d | |
| `EnnemiLemmings`, `EnnemiLemmingsVolant` | `bb::game::lemmings::Lemming` (`src/game/src/lemmings/lemmings.cpp`) | 5e | One class for ids 400 and 401 |
| `FondVentilo`, `FondLumiere`, `FondLumiere2`, `FondEau`, `FondEau1`, `FondEau2` | `Fan`, `FlickeringLight`, `Water`, `WaterTile` (`lemmings.cpp`) | 5e | |
| `EnnemiToad`, `EnnemiYoshi`, `EnnemiYoshiDCA`, `EnnemiKnukles` | `bb::game::video::Toad`, `Yoshi`, `YoshiTurret`, `Knuckles` (`src/game/src/video/walkers.cpp`) | 5f | Toad's "brain hurts" statics kept (D-50) |
| `EnnemiTails`, `EnnemiTailsvolant` | `Tails` (`walkers.cpp`) | 5f | One class, `entersFlying` for id 508 |
| `TirCarapace` | `Shell` (`walkers.cpp`) | 5f | |
| `EnnemiPrincesse`, `EnnemiPrincesseguard` | `Princess`, `PrincessGuard` (`princess.cpp`) | 5f | Guard holds a `Handle` (D-04) |
| `EnnemiRayman`, `TirRaymanpoing`, `MorceauRayman`, `MorceauTeteRayman` | `Rayman`, `RaymanFist`, `RaymanLimb`, `RaymanHead` (`rayman.cpp`) | 5f | Arena positions kept (D-51) |
| `EnnemiPikachuHeros`, `EnnemiBulbizarreHeros`, `EnnemiFlamecheHeros`, `EnnemiComHeros` | `PokemonHero` with four `HeroSpec`s (`rayman.cpp`) | 5f | |
| `FondCube`, `FondPiece`, `FondRing`, `FondTournesol1`–`4`, `FondDrapeau` | `SyncedProp` (block, coin, ring: shared clocks), `LoopProp` (`decor.cpp`) | 5f | |
| `EnnemiLara`, `EnnemiPic`, `EnnemiPoid`, `EnnemiTeteRayman`, `TirEpine` (Lara's) | `bb::game::videoboss::Lara`, `Spikes`, `Weight`, `RaymanHead`, `LaraSpine` (`src/game/src/videoboss/lara.cpp`) | 5g | `TirEpine` deleted (Sonic's uses were dead code, D-86) |
| `EnnemiMariotapette`, `EnnemiLuigi`, `TirDebouchechiote` | `SmallMario`, `Luigi` on `Plumber` (the shared push-back), `Plunger` (`plumbers.cpp`) | 5g | "Mariotapette" renamed (§ naming) |
| `EnnemiSonic`, `TirSoniceclair`, `MorceauSonicRing`, `EnnemiVoiturePacman` | `Sonic`, `SonicLightning`, `SonicRing`, `PacManCar` (`sonic.cpp`) | 5g | D-85 kept |
| `FondLCD`, `FondWorldmap`, `FondElectronic1`–`4`, `FondEcranneige`, `FondEcrancamera`, `FondAssembleur`, `FondWindows`, `FondDos`, `FondBossventilo` | `TableProp`, `Electronics`, `ScreenLoop`, `CameraScreen`, `Assembler`, `WindowsScreen` (`decor.cpp`) | 5g | |
| `GoreGiclure` | `bb::game::GoreDrop` (`src/game/src/gore.cpp`) | 5g | Shared: the cow bomb uses it too |
| `EnnemiMario`, `TirMarioFireball`, `TirMarioFireballVertical` | `bb::game::finalboss::Mario`, `Fireball`, `FireballUp` (`src/game/src/finalboss/mario.cpp`) | 5h | D-88 kept |
| `FondBattlecruiser`, `TirCruiser`, `TirCruiserLaser`, `FondRocherHyperspace` | `BattleCruiser`, `CruiserShot`, `CruiserLaser`, `HyperspaceRock` (`decor.cpp`) | 5h | D-87 kept |
| `FondMonolite`, `FondRocher1`, `FondRocher2` | `Floater` with three `FloaterSpec`s (`decor.cpp`) | 5h | Platform writes clipped (D-41) |
| `EventGenEnnemi`, `EventGenBonus`, `EventTexte`, `EventMusic`, `EventVehicule` | `EnemyGeneratorEvent`, `PickupGeneratorEvent`, `BannerTextEvent`, `MusicEvent`, `SpawnEvent` (vehicles) (`src/game/src/level_events.cpp`) | 5i | Every event type is ported |
| `GenEnnemi`, `GenEnnemiTMP`, `GenBonus`, `MakeBonus`, `MakeBonusWeapon`, `EventEnnemi`, `EventBonus` | `EnemyGenerator`, `PickupGenerator`, `dropPickup()`, `dropWeaponPickup()` (`src/game/src/generators.cpp`) | 5i | Statics → `World` fields |
| `Vehicule`, `VehiculeCigogne` | `bb::game::Vehicle` (`src/game/src/vehicle.cpp`), `finalboss::Stork` (`stork.cpp`) | 5i | The player is a `Rider` |
| `current_zik` | `World::currentMusic` | 5i | |
| `Couille`, `Blip`, `Blop`, `Joueur` | `bb::game::Player` with `Hero::Blip` / `Hero::Blop` (`src/game/src/player/player.cpp`) | 5j | Weapon tuning in one `WeaponSpec` table; D-89..D-91, D-93 kept |
| `Controlor`, `ControlP1`, `ControlP2` | `bb::game::PlayerControls`; legacy `LegacyControls` (`legacy_controls.h`) on the key aliases | 5j | The inertia switch was always on: gone |
| `TirBB`, `TirBBM16`, `TirBBPM`, `TirBBFusil`, `TirBBLaser`, `TirBBLF`, `TirBBVache` | `PlayerShot`, `RifleBullet`, `SmgBullet`, `ShotgunPellet`, `LaserBeam`, `Flame`, `FallingCow` (`player/shots.cpp`) | 5j | Pools kept; `dxReferentiel` was always 0: gone; D-92 kept |
| `Bonus`, `BonusPM`, `BonusFusil`, `BonusLaser`, `BonusLF`, `BonusBeer`, `BonusTonneau`, `BonusInvincible`, `BonusVache` | `WeaponPickup` with four specs, `Beer`, `Barrel`, `Invincibility`, `CowBomb` (`player/pickups.cpp`, `registerPickups()`) | 5j | |
| `Giclure`, `LevelEffects::spawnBlood()` | `bb::game::BloodSpray`, `spawnBlood()` (`src/game/src/effects.cpp`) | 5j | |
| `Bulle`, `Game::creeBulle()` | `Bubble`, `emitBubble()` (`effects.cpp`) | 5j | |
| `MeteoPluie`, `MeteoNeige`, the pools and `Game::updateMeteo()` | `Raindrop`, `Snowflake`, `Precipitation` (`effects.cpp`), owned by `Game` | 5j | |
| `Ennemi`, `tete_turc` (`enemy.cpp`) | deleted (no class derived from `Ennemi` any more); the target is `World::target` | 5j | |
| `cowBombOn` | `World::cowBombUnlocked` | 5j | |
| `TexteCool`, `clearTexteCool()`, `LevelEffects::showBanner()` | `bb::game::Banner`, `showBanner()`, `clearBanners()` (`src/game/src/banner.cpp`); `Canvas::drawText()` | 5j | Rows in `World::bannerRowFree` |
| `Sprite`, `Personnage`, `Tir`, `FondStatique` | deleted; `bb::game::Marker` for the target stand-in and the box of `grave()` | 5j | Nothing derives from them any more |
| `Game::tickBegin()` and the update/draw/clean methods, `updateScrolling()`, `worldStateHash()` | `bb::game::Level` (`src/game/src/level.cpp`) | 5k | D-94 kept |
| `Picture`, `PictureBank`, `Fonte`, `Surface::BltFast` of pictures | `bb::core::gfx::Sprite`, `SpriteBank`, `BitmapFont`, `draw()` | 5l | Legacy names are adapters until 5o |
| `scroll.cpp`, `videoA`, `grave()`, `num_decor` | `bb::game::Decor` (`src/game/src/decor.cpp`) | 5l | |
| `screens/hud.cpp` | `bb::game::Hud` (`src/game/src/hud.cpp`) | 5l | |
| `Sound`, `SoundBank`, `SoundBankBB`, `MusicBank` | `bb::game::SoundBank`, `MusicBank`, `AudioOutput` (`src/game/src/audio.cpp`) | 5m | |
| `ControlP1`/`ControlP2` (then `LegacyControls`) | `bb::game::AliasControls` on `InputState` | 5m | |
| `HiScores` | `bb::game::HiScores`; the file I/O stays at the edge | 5n | |
| `CINEPlayer`, `screens/cutscene_screen` | `bb::game::CutscenePlayer`, `CutsceneScreen` (`src/game/src/screens/cutscene.cpp`) | 5n | |
| `RPGPlayer` | `bb::game::DialoguePlayer` (`screens/dialogue.cpp`) | 5n | |
| `MenuMain`, `MenuGame`, `menu_wait.h`, `key_translator` | `bb::game::Menu`, `MainMenu`, `PauseMenu` (`screens/menus.cpp`), `MenuWait`; `core::input::displayName()` | 5n | |
| `WaitReleaseScreen`, `WaitKeyScreen`, `DelayScreen` | same names in `bb::game` (`screens/waits.cpp`) | 5n | |
| The other screens of `screens/` | `bb::game::CharacterSelectScreen`, `TitleScreen`, `HiScoreTableScreen`, `GameOverScreen`, `HiScoreEntryScreen`, `LevelSummaryScreen`, `CreditsScreen`, `AttractScreen`, `CampaignScreen`, `LevelScreen`, `PauseScreen`, `DialogueScreen` | 5n | |
| `Game` (`game.cpp`) | `bb::game::Game` (`src/game/src/game.cpp`) | 5n | |
| `event.h`, `fic_events.h`, `trace.h`, `ben_maths.*`, `lgx_packer.*`, `bb_rand.h`, `legacy_time.h` | deleted (no users left) | 5n | |
| `blip_n_blop_3.cpp` (InitApp, PlayGame, ReleaseAll), `legacy_media.cpp`, `legacy_assets.*`, `legacy_world.*`, `legacy_registry.*`, `video.*`, `screens/screen_runner.*` | `bb::app::App` (`src/app/app.cpp`) | 5o | |
| `session.*` | `bb::app::session` (`src/app/session.cpp`) | 5o | Owns the quit flag (`app_killed`) |
| `input.*` | `bb::app::DeviceInput` (`src/app/device_input.cpp`) | 5o | |
| `config.*` | `src/app/settings_file.cpp` | 5o | |
| `txt_data.*` | `bb::app::StringTable` (`src/app/strings.cpp`) | 5o | |
| `legacy_io.*` | `src/app/files.cpp` | 5o | `bbOpen` and the like had no users |
| `hi_scores.*` (file) | `src/app/app.cpp` | 5o | |
| `surface.*`, `picture.h`, `picture_bank.*`, `fonte.*`, `sound*.*`, `music_bank.*`, `ben_debug.*`, `legacy_types.h`, `legacy_app.h`, `control_alias.h`, `zero_new.cpp` | deleted | 5o | Core and game types used directly; `core::Log` |
| `ennemi_mario_hologramme.*` | deleted, described in `docs/cut_content.md` | 5h | Never spawned |
| `vehicule_hypo.*`, `vehicule_test_tourelle.h` | deleted, described in `docs/cut_content.md` | 5i | Unreachable |
| `ennemi_bisou_coeur.*`, `tir_coeur_bisou.*`, `gore_tete_bisou_coeur.h`, `ennemi_bisou_auto.*`, `gore_bisou_auto.h`, `explosion.*` | deleted, described in `docs/cut_content.md` | 6 (done during 5) | Heart bear (206), car bear: never placed |
| `ennemi_diabolo.*`, `tir_tornade.*`, `ennemi_pacman.*` | deleted, described in `docs/cut_content.md` | 6 (done during 5) | Diabolo (306) never placed; Pac-Man never spawned |
| `fond_etoile.*`, `fond_poissons.*`, `fond_snork_sas.*`, `fond_test.h`, `p_plan_test.h`, `impactlaser.h` | deleted, described in `docs/cut_content.md` | 6 (done during 5) | Props 705, 300–305 never placed; test headers never included |
