#pragma once

#include "../Entity.hpp"
#include "../Components.hpp"
#include "../../main/Rect.hpp"
#include "../../rendering/Sprites.hpp"
#include "../../inventory/Recipe.hpp"

#include <glm/glm.hpp>

class Dragon : public Entity, public IInteractable, public IInventoryHolder, public IProgress{
    /* Dragon implementation for demo. */
public:
    Dragon();
    ~Dragon() = default;

    // Entity
    void update(FrameInput const &input, float elapsed, World const &world) override;
	auto hitbox() const -> Rect override;
    void draw(Sprites &batch) const override;
    void get_occupied(std::vector<glm::ivec2> *ans) const override;

    // IInteractable
    void on_interact(Context &ctx) override;

    // IInventoryHolder
    auto get_inventory() -> Inventory* override;

    // IProgress
    float get_progress() const override;

private:
    Inventory inventory;
    float processing_timer = 0.0f;
    const Recipe *current_recipe = nullptr;
};