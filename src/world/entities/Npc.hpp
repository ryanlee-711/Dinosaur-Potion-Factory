#pragma once

#include "../Entity.hpp"
#include "../Components.hpp"
#include "../../inventory/Inventory.hpp"
#include "../../main/Rect.hpp"
#include "../../rendering/Sprites.hpp"
#include "../../rendering/Texture.hpp"
#include "../../starter/Load.hpp"

#include <glm/glm.hpp>

#include <string>
#include <vector>

// A character you can talk to. Talking opens the dialogue box. An npc can give one side quest and/or trade.
//
// To add a new one: a sprite sheet, one line in the table in Npc.cpp, its lines in dialogue.tsv,
// its side quest (if any) in PlayMode's constructor, and its type name to the npc branch in PlayMode::load.
struct NpcDef {
    std::string type;          // name in the save file
    std::string dialogue;      // npc id in dialogue.tsv
    std::string side_quest;    // id of the side quest it gives ("" = none)
    ItemRecord trade_in;       // trade: put this many in... (empty id = doesn't trade)
    ItemRecord trade_out;      // ...get this many out
    bool trade_after_quest;    // trade only opens once its side quest is done
    float trade_seconds;       // time to make one trade_out (0 = instant)
    Load< Texture > *texture;  // 4 rows (facing right, up, left, down) x 2 idle frames
    glm::vec2 frame_size;      // one frame, in pixels; the feet are at the bottom center

    bool trades() const { return !trade_in.first.empty(); }
    static NpcDef const *get(std::string const &type);
};

class Npc : public Entity, public IInteractable, public IHintable, public IProgress {
public:
    explicit Npc(std::string const &type = "lumberjack");
    ~Npc() = default;

    // Entity. position = the npc's feet.
    void update(FrameInput const &input, float elapsed, World const &world) override;
    auto hitbox() const -> Rect override;
    void draw(Sprites &batch) const override;
    void get_occupied(std::vector<glm::ivec2> *ans) const override;

    // Save line: <uid> <type> <feet_x> <feet_y> [met] [trade slot 0: id count] [trade slot 1: id count]
    std::string save() const override;
    void load(std::string line) override;

    // IInteractable: turns to face the dwarf. PlayMode opens the dialogue box.
    void on_interact(Context &ctx) override;

    // IHintable: the E key
    bool show_hint() const override { return true; }
    item_id_t hint_tool() const override { return ""; }
    glm::vec2 hint_anchor() const override;

    // IProgress: fills up while a trade is being made
    float get_progress() const override;

    // Where the "!" floats, world pixels
    glm::vec2 marker_anchor() const;

    NpcDef const &definition() const { return *def; }
    Inventory &trade_inventory() { return trade; }

    bool met = false;  // talked to at least once

protected:
    void set_def(std::string const &type);
    void face(glm::vec2 point);

    NpcDef const *def = nullptr;
    Inventory trade{2};             // slot 0: what you give, slot 1: what you get
    glm::ivec2 facing = glm::ivec2(0, 1);
    float idle_timer = 0.0f;
    float trade_timer = 0.0f;       // seconds into the current trade (timed trades)
};