#pragma once

#include "../Entity.hpp"
#include "../Components.hpp"
#include "../Player.hpp"
#include "../../main/Rect.hpp"
#include "../../rendering/Sprites.hpp"
#include "../../rendering/Texture.hpp"
#include "../../starter/Load.hpp"

#include <glm/glm.hpp>

#include <string>
#include <vector>

struct World;
struct ParticlePreset;

// The dwarf's sprint speed: creature speeds are set relative to it.
inline constexpr float PLAYER_SPRINT_SPEED = Player::WALK_SPEED * Player::SPRINT_MULTIPLIER;

// One kind of creature. To add a new one: add a texture and one entry to the table in Creature.cpp,
// then add its type name to the creature branch in PlayMode::load.
struct CreatureDef {
    std::string type;              // name in the save file
    item_id_t drop;                // what pressing E on it gives
    int max_drops;                 // how many it carries at once
    float regrow_seconds;          // seconds to grow back one
    std::string empty_message;     // E with nothing left, e.g. "It has no feathers left to pluck"
    float wander_speed;            // pixels/second while wandering
    float flee_speed;              // pixels/second while running away
    float notice_radius;           // starts fleeing when the dwarf is this close (pixels)
    float calm_radius;             // stops fleeing when the dwarf is farther than this (pixels)
    glm::vec2 hitbox_size;         // around the feet (also what E has to reach)
    Load< Texture > *texture;      // 16x16 frames, see the sheet layout in the Creature comment
    ParticlePreset const *harvest_particles; // burst when you take something from it (nullptr = none)

    static CreatureDef const *get(std::string const &type); // nullptr if unknown
};

class Creature : public Entity, public IInteractable, public IHintable {
    /* A creature that roams inside its zone (a rectangle of tiles set in the level file):

       Behavior:
         Idle    Stands still for a bit, then picks a random spot in the zone and walks there
         Flee    The dwarf came within notice_radius: runs away at flee_speed.
                 Changes directions to make sure the creature doesn't go outside its zone
                 Calms down (back to Idle) once the dwarf is farther than calm_radius.
         It never leaves its zone or walks into solid tiles / solid things (trees, boulders, machines...).
       Catching: face it within reach and press E to take its drop. It carries max_drops and regrows them.

       Sprite sheet (16x16 frames): row 0 faces right, row 1 faces left;
       columns 0 = standing, 1-2 = running steps, 3 (row 0) = the "!" shown when it gets startled. */
public:
    explicit Creature(std::string const &type = "vulmanid");
    ~Creature() = default;

    // Entity
    void update(FrameInput const &input, float elapsed, World const &world) override;
    auto hitbox() const -> Rect override;
    void draw(Sprites &batch) const override;
    void get_occupied(std::vector<glm::ivec2> *ans) const override;

    // Save line: <uid> <type> <x> <y> <zone_left> <zone_top> <zone_right> <zone_bottom> [drops]
    //   type = "vulmanid", "glaber", ...    x y = feet position in pixels (in the level file: where it spawns)
    //   zone = the tiles it may roam, inclusive (e.g. 208 60 224 76). The spawn point must be inside.
    std::string save() const override;
    void load(std::string line) override;

    // IInteractable
    void on_interact(Context &ctx) override;

    // IHintable: E key while it has something to take
    bool show_hint() const override { return drops > 0; }
    item_id_t hint_tool() const override { return ""; }
    glm::vec2 hint_anchor() const override { return glm::round(position) + glm::vec2(0.0f, -17.0f); }

    // The roaming zone in world pixels (PlayMode outlines it in the F3 debug view).
    Rect zone_rect() const;

    // ---- tuning shared by all creatures ----
    static constexpr float MAX_FLEE_TURN = 105.0f;        // degrees it may turn away from "straight away from the dwarf"
    static constexpr float WANDER_DISTANCE = 64.0f;       // how far (pixels) a wander trip goes at most, per axis
    static constexpr float TURN_AROUND_DISTANCE = 3.0f;   // pixels of steady sideways movement before the sprite flips

private:
    CreatureDef const *def = nullptr;

    enum State : int { Idle = 0, Wander = 1, Flee = 2 };
    State state = Idle;
    float state_timer = 1.0f;          // Idle: seconds until it wanders off
    glm::vec2 wander_target = glm::vec2(0.0f);
    int turn_side = 1;
    glm::vec2 flee_dir = glm::vec2(0.0f); // direction it's running

    glm::ivec2 zone_min = glm::ivec2(0), zone_max = glm::ivec2(0);

    int drops = 0;
    float regrow_timer = 0.0f;

    // looks
    bool moving = false;
    bool facing_left = false;
    float turn_around = 0.0f;          // sideways distance moved against facing; it turns around past TURN_AROUND_DISTANCE
    float step_distance = 0.0f;        // walked distance, drives the running animation
    float alarm_timer = 0.0f;          // > 0: show the "!" above it
    bool harvested_pending = false;    // something was just taken: particle burst next update

    void set_def(std::string const &type);
    Rect hitbox_at(glm::vec2 feet) const;
    bool can_stand(glm::vec2 feet, World const &world) const;
    bool move(glm::vec2 step, World const &world);
    void start_fleeing();
};