#pragma once

// bbedit's window: the level canvas, the tools, the inspector, the level's
// properties and the sprite banks' hotspots (docs/editor.md).

#include <atomic>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "bb/game/entity_registry.hpp"
#include "json_text.hpp"
#include "level_document.hpp"
#include "textures.hpp"

struct SDL_Renderer;
struct ImDrawList;

namespace bb::editor {

class EditorApp {
public:
    // sourceDir: the source asset tree (assets/source). toolDir: where
    // bbassets and the game are looked for (test play).
    EditorApp(SDL_Renderer* renderer, std::string sourceDir, std::string toolDir);
    ~EditorApp();
    EditorApp(const EditorApp&) = delete;
    EditorApp& operator=(const EditorApp&) = delete;

    // Opens levels/<stem>.json; false and a message when it cannot.
    bool open(const std::string& stem);
    // Builds this frame's interface.
    void frame();
    // The window's close button: asks first when something is not saved.
    void requestClose();
    bool done() const noexcept { return done_; }
    // Shows the level from this x, at this zoom.
    void lookAt(float x, float zoom);
    // Opens a part of the interface (--show, for screenshots): "place",
    // "platforms", "level", "sprites" (the level's enemy bank), "select"
    // (the first enemy).
    void show(const std::string& part);
    // For the self-test: the level, and where a level point is drawn.
    const LevelDocument* document() const { return doc_.get(); }
    float screenX(float wx) const { return toScreenX(wx); }
    float screenY(float wy) const { return toScreenY(wy); }

private:
    enum class Tool { Select, Place, Platform, Wall, Start1, Start2, Victory };
    enum class Selected { None, Event, Start1, Start2, Victory };
    struct Selection {
        Selected kind = Selected::None;
        int index = -1;  // the event
    };
    // A drawn object the mouse can pick, in screen coordinates.
    struct Hit {
        Selection what;
        float x1, y1, x2, y2;
    };

    // Interface parts.
    void shortcuts();
    void menuBar();
    void toolPanel(float x, float y, float width, float height);
    void inspector(float x, float y, float width, float height);
    void canvas(float x, float y, float width, float height);
    void statusBar(float x, float y, float width, float height);
    void levelWindow();
    void spriteWindow();
    void newLevelPopup();
    void unsavedPopup();
    void eventFields(LevelEventRecord& e, bool& changed);
    void palette();

    // Canvas drawing and editing.
    void drawLevel(ImDrawList* dl);
    void drawEvent(ImDrawList* dl, int index, const LevelEventRecord& e);
    void drawPicture(ImDrawList* dl, const Picture& p, float wx, float wy, std::uint32_t tint);
    void drawBox(ImDrawList* dl, float wx, float wy, const std::string& label, std::uint32_t colour);
    void canvasInput(bool hovered);
    void paintStroke(float wx, float wy, bool erase);
    void select(Selection s);
    void deleteSelection();
    void duplicateSelection();

    // Coordinates.
    float toScreenX(float wx) const { return originX_ + (wx - cameraX_) * zoom_; }
    float toScreenY(float wy) const { return originY_ + (wy - cameraY_) * zoom_; }
    float toWorldX(float sx) const { return cameraX_ + (sx - originX_) / zoom_; }
    float toWorldY(float sy) const { return cameraY_ + (sy - originY_) / zoom_; }
    void zoomAt(float factor, float sx, float sy);
    void showWhole();

    // Files.
    void scanSourceTree();
    void loadPreviews();
    bool save();
    // Runs `action` now, or after asking to save the changes.
    void whenSaved(std::function<void()> action);
    // Saves and plays the level in the game: alone in an overlay pack over
    // the game's assets, or (rebuild) with every asset packed again from
    // the source tree (sprites changed too).
    void playLevel(bool rebuild);
    const Picture* entityPicture(game::EntityCategory category, std::int32_t id);
    std::string entityName(game::EntityCategory category, std::int32_t id) const;
    std::string worldOfLevel() const;
    void status(std::string text, bool error = false);

    std::string sourceDir_;
    std::string toolDir_;
    Textures textures_;
    bool done_ = false;

    // The source tree.
    std::vector<std::string> levels_;
    std::vector<std::string> spriteBanks_;  // "snufenn.gfx"
    std::vector<std::string> soundBanks_;   // "smurf.sfx"
    std::vector<std::string> playlists_;    // "snuf.mbk"
    std::vector<std::string> dialogues_;    // "snuff.rpg"
    std::map<std::string, std::pair<std::string, int>> previews_;  // entity -> bank, image

    // The level.
    std::unique_ptr<LevelDocument> doc_;  // null: no level open
    std::string stem_;
    Selection selection_;
    std::vector<Hit> hits_;

    // View.
    float cameraX_ = 0;
    float cameraY_ = 0;
    float zoom_ = 1;
    float originX_ = 0;
    float originY_ = 0;
    float viewWidth_ = 0;
    float viewHeight_ = 0;
    bool fitPending_ = true;
    bool showDecor_ = true;
    bool showPlatforms_[LevelData::kPlatformLayers] = {true, true, true, true, true, true, true, true};
    bool showSolidWalls_ = true;
    bool showBloodWalls_ = true;
    bool showEntities_ = true;
    bool showDecals_ = true;
    bool showLineEvents_ = true;

    // Tools.
    Tool tool_ = Tool::Select;
    std::int32_t placeType_ = 0;      // LevelEventType
    std::int32_t placeEntity_ = 0;    // its entity's 2002 id
    std::int32_t placeImage_ = 0;     // a decal's image
    std::string paletteFilter_;
    int platformLayer_ = 0;
    int brushWidth_ = 4;
    bool wallBlood_ = false;
    int wallBrush_ = 1;

    // Mouse gestures.
    bool dragging_ = false;
    bool painting_ = false;
    bool paintErase_ = false;
    float lastPaintX_ = 0;
    float lastPaintY_ = 0;
    float dragStartX_ = 0;
    float dragStartY_ = 0;
    std::int32_t dragOriginX_ = 0;
    std::int32_t dragOriginY_ = 0;

    // Windows and popups.
    bool levelWindowOpen_ = false;
    bool spriteWindowOpen_ = false;
    bool newLevelRequested_ = false;
    std::string newLevelName_;
    std::function<void()> pendingAction_;
    std::string levelToOpen_;  // chosen in File > Open level
    bool unsavedRequested_ = false;
    std::string screensText_;
    bool screensTextFresh_ = false;
    std::string markerName_;
    std::string markerValues_;

    // Sprite bank editing (sprites/<stem>/sprites.json).
    std::string spriteBank_;
    int spriteImage_ = 0;
    int spriteZoom_ = 4;
    tools::Json spriteManifest_;  // the manifest being edited
    bool spriteDirty_ = false;

    // Status line and test play.
    std::string status_;
    bool statusError_ = false;
    std::thread player_;
    std::atomic<int> playState_{0};  // 0 idle, 1 running, 2 finished, 3 failed
};

}  // namespace bb::editor
