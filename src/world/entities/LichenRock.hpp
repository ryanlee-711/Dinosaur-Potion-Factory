#pragma once

#include "../Entity.hpp"
#include "../Components.hpp"
#include "../../main/Rect.hpp"
#include "../../rendering/Sprites.hpp"

#include <glm/glm.hpp>

class LichenRock : public Entity, public IInteractable {
    /* A rock that grows lichen: face it and press E to pick all the lichen on it (straight into the bag).
       It holds a few at a time and slowly regrows them. */
public:
    LichenRock() = default;
    ~LichenRock() = default;

    // Entity
    void update(FrameInput const &input, float elapsed, World const &world) override;
	auto hitbox() const -> Rect override;
    void draw(Sprites &batch) const override;
    void get_occupied(std::vector<glm::ivec2> *ans) const override;

    // IInteractable
    void on_interact(Context &ctx) override;

private:
    static constexpr int MAX_LICHEN = 5;            // lichen the rock can hold at once
    static constexpr float REGROW_COOLDOWN = 5.0f;  // seconds to grow back one
    int lichen = MAX_LICHEN;                        // lichen ready to pick right now
    float regrow_timer = REGROW_COOLDOWN;
};