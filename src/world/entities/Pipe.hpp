#pragma once

#include "../Entity.hpp"
#include "../Components.hpp"
#include "../../main/Rect.hpp"
#include "../../rendering/Sprites.hpp"

#include <glm/glm.hpp>

class Pipe : public Entity, public IInteractable, public IInventoryHolder, public IItemPort {
public:
    Pipe();
    ~Pipe() = default;

    // Entity
    void update(FrameInput const &input, float elapsed, Tilemap const &map, std::vector<std::unique_ptr<Entity>> const &entities) override;
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
    glm::ivec2 in_dir;
    glm::ivec2 out_dir;
    size_t frame;
    
    const float PUSH_COOLDOWN = 0.2f;
    float push_timer = 0.0f;
};