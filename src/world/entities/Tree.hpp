#pragma once

#include "../Entity.hpp"
#include "../Components.hpp"
#include "../../main/Rect.hpp"
#include "../../rendering/Sprites.hpp"

#include <glm/glm.hpp>

#include <string>
#include <vector>

class Tree : public Entity, public IMineable, public IHintable {
    /* A tree you chop with the axe. Each click is one hit (more with the strength effect); after
       HITS_TO_FELL hits it falls over away from you, drops lumber where it lands and leaves a stump.
       The stump regrows into a full tree after REGROW_SECONDS. The trunk (2 tiles) is solid.

       Sprite sheet: frames are FRAME_W x FRAME_H, the trunk's bottom-center sits at (PIVOT_X, PIVOT_Y) in every frame.
         columns: 0 standing, 1-3 falling (more and more tilted), 4 stump
         rows:    0 falls to the right, 1 falls to the left */
public:
    Tree() = default;
    ~Tree() = default;

    // Entity
    void update(FrameInput const &input, float elapsed, World const &world) override;
    auto hitbox() const -> Rect override;
    void draw(Sprites &batch) const override;
    void get_occupied(std::vector<glm::ivec2> *ans) const override;

    // Save line: <uid> tree <x> <y> <state> <hits> <fall_dir> <timer>
    std::string save() const override;
    void load(std::string line) override;

    // IMineable
    item_id_t required_tool() const override { return "axe"; }
    bool mine(int power, glm::vec2 from, std::vector<ItemRecord> &drops) override;
    bool restore();
    std::string empty_message() const override { return "Just a stump. It will grow back"; }

    // IHintable: axe icon, only on a standing tree (not while falling, not a stump)
    bool show_hint() const override { return state == Standing; }
    item_id_t hint_tool() const override { return required_tool(); }
    glm::vec2 hint_anchor() const override { return base() + glm::vec2(0.0f, -49.0f); } //above the crown

    static constexpr int HITS_TO_FELL = 6;
    static constexpr int LUMBER_DROPS = 3;          // logs dropped when it lands
    static constexpr float FALL_SECONDS = 0.5f;     // falling animation
    static constexpr float REGROW_SECONDS = 90.0f;  // stump -> full tree
    static constexpr float SHAKE_SECONDS = 0.15f;   // wobble after each hit

    static constexpr float FRAME_W = 112.0f, FRAME_H = 80.0f;
    static constexpr float PIVOT_X = 56.0f, PIVOT_Y = 64.0f;

private:
    enum State : int { Standing = 0, Falling = 1, Stump = 2 };
    State state = Standing;
    int hits = 0;
    int fall_dir = 1;            // +1 falls right, -1 falls left
    float timer = 0.0f;          // Falling: time since it started falling. Stump: time left until it regrows.
    float shake_timer = 0.0f;
    bool chips_pending = false;  // a hit landed: spray wood chips next update

    glm::vec2 base() const;
};