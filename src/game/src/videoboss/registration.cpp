#include "bb/game/videoboss_world.hpp"
#include "videoboss/videoboss_world.hpp"

namespace bb::game {

namespace {

namespace vb = videoboss;
namespace prop = videoboss::prop;

template <class T>
Entity* make(World& world, const SpawnPoint& at) {
    auto* entity = new T(world);
    entity->x = at.x;
    entity->y = at.y;
    entity->direction = at.direction;
    return entity;
}

template <class T>
Entity* place(World& world, const SpawnPoint& at) {
    auto* entity = new T(world);
    entity->x = at.x;
    entity->y = at.y;
    return entity;
}

// The LCD screen and the world map (steps from the first image).
const int kLcd[] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 10, 11, 11, 10, 10, 11, 11,
                    10, 10, 11, 11, 10, 10, 11, 11, 10, 10, 12, 12, 12, 12, 12, 12, 12};
const int kWorldMap[] = {0,  1,  0,  1,  0,  1,  2,  3,  4,  5,  6,  7,  6,  7,  6,  7,  8, 9,
                         10, 11, 12, 13, 12, 13, 12, 13, 12, 13, 13, 13, 13, 13, 13, 13, 13};

template <int kFirst, const int* kTable, int kDelay>
Entity* makeTable(World& world, const SpawnPoint& at) {
    auto* entity = new vb::TableProp(world, kFirst, kTable, 35, kDelay);
    entity->x = at.x;
    entity->y = at.y;
    return entity;
}

template <int kOverlay>
Entity* makeElectronics(World& world, const SpawnPoint& at) {
    auto* entity = new vb::Electronics(world, kOverlay);
    entity->x = at.x;
    entity->y = at.y;
    return entity;
}

template <int kFirst, int kSteps, int kDelay>
Entity* makeLoop(World& world, const SpawnPoint& at) {
    auto* entity = new vb::ScreenLoop(world, kFirst, kSteps, kDelay);
    entity->x = at.x;
    entity->y = at.y;
    return entity;
}

void addEnemy(EntityRegistry& registry, int id, const char* name, EntityFactory factory) {
    registry.add({EntityCategory::Enemy, id, name, Placement::Enemies, factory});
}

void addProp(EntityRegistry& registry, int id, const char* name, EntityFactory factory) {
    registry.add({EntityCategory::Prop, id, name, Placement::Props, factory});
}

}  // namespace

void registerVideoBossWorld(EntityRegistry& registry) {
    addEnemy(registry, 600, "videoboss.luigi", make<vb::Luigi>);
    addEnemy(registry, 601, "videoboss.sonic", make<vb::Sonic>);
    addEnemy(registry, 602, "videoboss.small_mario", make<vb::SmallMario>);
    addEnemy(registry, 603, "videoboss.lara", make<vb::Lara>);
    addEnemy(registry, 604, "videoboss.spikes", make<vb::Spikes>);
    addEnemy(registry, 605, "videoboss.weight", make<vb::Weight>);

    addProp(registry, 600, "videoboss.lcd", makeTable<prop::kLcd, kLcd, 4>);
    addProp(registry, 601, "videoboss.world_map", makeTable<prop::kWorldMap, kWorldMap, 10>);
    addProp(registry, 602, "videoboss.electronics_1", makeElectronics<prop::kElectronicsOverlay1>);
    addProp(registry, 603, "videoboss.electronics_2", makeElectronics<prop::kElectronicsOverlay2>);
    addProp(registry, 604, "videoboss.electronics_3", makeElectronics<prop::kElectronicsOverlay3>);
    addProp(registry, 605, "videoboss.electronics_4", makeElectronics<-1>);
    addProp(registry, 606, "videoboss.snow_screen", makeLoop<prop::kSnowScreen, 4, 4>);
    addProp(registry, 607, "videoboss.camera_screen", place<vb::CameraScreen>);
    addProp(registry, 608, "videoboss.assembler", place<vb::Assembler>);
    addProp(registry, 609, "videoboss.windows", place<vb::WindowsScreen>);
    addProp(registry, 610, "videoboss.dos", makeLoop<prop::kDos, 2, 40>);
    addProp(registry, 611, "videoboss.fan", makeLoop<prop::kFan, 4, 3>);
}

}  // namespace bb::game
