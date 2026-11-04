#pragma once

#include "Entity.hpp"
#include "Components.hpp"
#include "../main/Rect.hpp"
#include "../rendering/Sprites.hpp"

#include <glm/glm.hpp>

class ItemEntity : public Entity, public IInventoryHolder {
    /* Chest implementation for demo. */
public:
    ItemEntity();
    ItemEntity(item_id_t item_id, int item_count);
    ~ItemEntity() = default;

    // Entity
    void update(FrameInput const &input, float elapsed, World const &world) override;
	auto hitbox() const -> Rect override;
    void draw(Sprites &batch) const override;
    void get_occupied(std::vector<glm::ivec2> *ans) const override;

    // IInventoryHolder
    auto get_inventory() -> Inventory* override;

    // Item
    auto is_absorbable() -> bool;

    glm::vec2 velocity = glm::vec2{0.0f};

private:
    Inventory inventory;
    bool ready_to_absorb = false;     // When set to true
    float no_absorb_cooldown = 0.0f;  // A cooldown after spawning that prevents the item from being collected
    float height = 0.0f;              // a pseudo-3D height for the thing
    float vertical_velocity = 0.0f;   // vertical velocity (down is positive)
};