#pragma once

#include "../Entity.hpp"
#include "../Components.hpp"
#include "../../main/Rect.hpp"
#include "../../rendering/Sprites.hpp"

#include <glm/glm.hpp>

class AppleTree : public Entity, public IInteractable, public IInventoryHolder {
    /* AppleTree implementation for demo. */
public:
    AppleTree();
    ~AppleTree() = default;

    // Entity
    void update(FrameInput const &input, float elapsed, World const &world) override;
	auto hitbox() const -> Rect override;
    void draw(Sprites &batch) const override;
    void get_occupied(std::vector<glm::ivec2> *ans) const override;

    // IInteractable
    void on_interact(Context &ctx) override;

    // IInventoryHolder
    auto get_inventory() -> Inventory* override;


private:
    Inventory inventory;
    const float REGROW_COOLDOWN = 2.0f;
    float regrow_timer = 0.0f;
};