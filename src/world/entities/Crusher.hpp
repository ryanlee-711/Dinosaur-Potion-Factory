#pragma once

#include "../Entity.hpp"
#include "../Components.hpp"
#include "../../main/Rect.hpp"
#include "../../rendering/Sprites.hpp"

#include <glm/glm.hpp>

class Crusher : public Entity, public IInteractable, public IInventoryHolder, public IItemPort, public IBreakable {
    /* Crusher implementation for demo. */
public:
    Crusher();
    ~Crusher() = default;

    // Entity
    void update(FrameInput const &input, float elapsed, World const &world) override;
	auto hitbox() const -> Rect override;
    void draw(Sprites &batch) const override;
    void get_occupied(std::vector<glm::ivec2> *ans) const override;

    // IInteractable
    void on_interact(Context &ctx) override;

    // IInventoryHolder
    auto get_inventory() -> Inventory* override;

    // IItemPort
    PortType get_port(glm::ivec2 grid_coord) override;
    int push_item(ItemRecord item_record) override;


private:
    Inventory inventory;
    const float PUSH_COOLDOWN = 1.0f;
    const float CRUSH_COOLDOWN = 3.0f;
    float push_timer = 0.0f;
    float processing_timer = 0.0f;
    size_t frame = 0;
};