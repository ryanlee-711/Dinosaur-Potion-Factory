#pragma once

#include "../Entity.hpp"
#include "../Components.hpp"
#include "../../main/Rect.hpp"
#include "../../rendering/Sprites.hpp"

#include <glm/glm.hpp>

#include "../../rendering/Texture.hpp"

class Chest : public Entity, public IInteractable, public IInventoryHolder {
    /* Chest implementation for demo. */
public:
    ~Chest() = default;

    // Entity
    void update(FrameInput const &input, float elapsed, Tilemap const &map) override;
	auto hitbox() const -> Rect override;
    void draw(Sprites &batch) const override;
    void get_occupied(std::vector<glm::ivec2> *ans) const override;

    // IInteractable
    void on_interact(Context &ctx) override;

    // IInventoryHolder
    auto get_inventory() -> Inventory* override;


private:
    Inventory inventory;
};