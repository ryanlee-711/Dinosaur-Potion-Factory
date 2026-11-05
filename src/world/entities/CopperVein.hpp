#pragma once

#include "../Entity.hpp"
#include "../Components.hpp"
#include "../../main/Rect.hpp"
#include "../../rendering/Sprites.hpp"

#include <glm/glm.hpp>

class CopperVein : public Entity, public IInteractable, public IInventoryHolder, public IBreakable {
    /* CopperVein implementation for demo. */
public:
    CopperVein();
    ~CopperVein() = default;

    // Entity
    void update(FrameInput const &input, float elapsed, World const &world) override;
	auto hitbox() const -> Rect override;
    void draw(Sprites &batch) const override;
    void get_occupied(std::vector<glm::ivec2> *ans) const override;

    // IInteractable
    void on_interact(Context &ctx) override;

    // IInventoryHolder
    auto get_inventory() -> Inventory* override;

    // IBreakable
    int get_hardness() override {return 99;}
    item_id_t get_drop_id() override {return "copper_ore";}


private:
    Inventory inventory;
    const float REGROW_COOLDOWN = 8.0f;
    float regrow_timer = 0.0f;
};