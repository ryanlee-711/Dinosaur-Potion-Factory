#pragma once

#include "../Entity.hpp"
#include "../Components.hpp"
#include "../Player.hpp"
#include "../../main/Rect.hpp"
#include "../../rendering/Sprites.hpp"

#include <glm/glm.hpp>

#include <string>
#include <vector>

struct World;

class Vulmanid : public Entity, public IInteractable, public IHintable {
    /* A flightless feathered creature that roams inside its zone (a rectangle of tiles set in the level file).

       AI:
         Idle    Stands still for a bit, then picks a random spot in the zone and walks there.
         Flee    The dwarf came within NOTICE_RADIUS: runs straight away at FLEE_SPEED.
                 Changes direction to avoid going into zone border
                 If no direction works it's cornered and stands still: that's your chance.
                 Calms down (back to Idle) once the dwarf is farther than CALM_RADIUS.
       Catching: face it within reach and press E to pluck a feather. It holds MAX_FEATHERS and regrows them.
    */
public:
    Vulmanid() { collision_mask = 0x0; } //you can run through it
    ~Vulmanid() = default;

    // Entity
    void update(FrameInput const &input, float elapsed, World const &world) override;
    auto hitbox() const -> Rect override;
    void draw(Sprites &batch) const override;
    void get_occupied(std::vector<glm::ivec2> *ans) const override;

    // Save line: <uid> vulmanid <x> <y> <zone_left> <zone_top> <zone_right> <zone_bottom> [feathers]
    //   x y = feet position in pixels (in the level file: where it spawns)
    //   zone = the tiles it may roam, inclusive (e.g. 208 60 224 76). The spawn point must be inside.
    std::string save() const override;
    void load(std::string line) override;

    // IInteractable
    void on_interact(Context &ctx) override;

    // IHintable: E key while it has a feather to pluck
    bool show_hint() const override { return feathers > 0; }
    item_id_t hint_tool() const override { return ""; }
    glm::vec2 hint_anchor() const override { return glm::round(position) + glm::vec2(0.0f, -17.0f); }

    // The roaming zone in world pixels (PlayMode outlines it in the F3 debug view).
    Rect zone_rect() const;

    // ---- tuning ----
    static constexpr float WANDER_SPEED = 28.0f;                                         // pixels/second
    static constexpr float FLEE_SPEED = Player::WALK_SPEED * Player::SPRINT_MULTIPLIER;  // the dwarf's sprint
    static constexpr float NOTICE_RADIUS = 56.0f;   // starts fleeing when the dwarf is this close (3.5 tiles)
    static constexpr float CALM_RADIUS = 112.0f;    // stops fleeing when the dwarf is farther than this (7 tiles)
    static constexpr float MAX_FLEE_TURN = 105.0f;  // degrees it may turn away from "straight away from the dwarf"
    static constexpr int MAX_FEATHERS = 2;
    static constexpr float FEATHER_REGROW_SECONDS = 20.0f;
    static constexpr float HITBOX_W = 12.0f, HITBOX_H = 8.0f;
    static constexpr float TURN_AROUND_DISTANCE = 3.0f; // pixels of steady sideways movement before the sprite flips

private:
    enum State : int { Idle = 0, Wander = 1, Flee = 2 };
    State state = Idle;
    float state_timer = 1.0f;          // Idle: seconds until it wanders off
    glm::vec2 wander_target = glm::vec2(0.0f);
    int turn_side = 1;                 // which way it turned last while fleeing (+1 / -1)
    glm::vec2 flee_dir = glm::vec2(0.0f); // direction it's running; kept while it still works, so it doesn't wobble

    glm::ivec2 zone_min = glm::ivec2(0), zone_max = glm::ivec2(0); // tiles, inclusive

    int feathers = MAX_FEATHERS;
    float regrow_timer = FEATHER_REGROW_SECONDS;

    // looks
    bool moving = false;
    bool facing_left = false;
    float turn_around = 0.0f;          // sideways distance moved against facing; it turns around past TURN_AROUND_DISTANCE
    float step_distance = 0.0f;        // walked distance, drives the running animation
    float alarm_timer = 0.0f;          // > 0: show the "!" above it
    bool plucked_pending = false;      // a feather was just plucked: feather puff next update

    Rect hitbox_at(glm::vec2 feet) const;
    bool can_stand(glm::vec2 feet, World const &world) const; // inside the zone and not in anything solid
    bool move(glm::vec2 step, World const &world);
    void start_fleeing();
};