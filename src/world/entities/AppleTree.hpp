#pragma once

#include "../Entity.hpp"
#include "../Components.hpp"
#include "../../main/Rect.hpp"
#include "../../rendering/Sprites.hpp"

#include <glm/glm.hpp>

class AppleTree : public Entity {
    /* An apple tree: every few seconds it drops an apple on the ground somewhere around it,
       until enough apples are lying nearby. Apples are picked up like any other ground item. */
public:
    AppleTree() = default;
    ~AppleTree() = default;

    // Entity
    void update(FrameInput const &input, float elapsed, World const &world) override;
	auto hitbox() const -> Rect override;
    void draw(Sprites &batch) const override;
    void get_occupied(std::vector<glm::ivec2> *ans) const override;

private:
    static constexpr float DROP_COOLDOWN = 15.0f;   // seconds between apples
    static constexpr int MAX_NEARBY_APPLES = 5;    // stop dropping once this many apples lie around the tree
    static constexpr float DROP_RADIUS = 1.5f * GameConfig::TILE_SIZE;  // apples land up to ~2 tiles away
    static constexpr float COUNT_RADIUS = 3.0f * GameConfig::TILE_SIZE; // apples this close count as "nearby"

    float drop_timer = DROP_COOLDOWN;
    int nearby_apples = 0;  // counted every update; also picks the sprite (fruit while it can still drop)

    glm::vec2 base_center() const; // middle of the tree's 2x1-tile base, in world pixels
};