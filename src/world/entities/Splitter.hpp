#pragma once

#include "../Entity.hpp"
#include "../Components.hpp"
#include "../../main/Rect.hpp"
#include "../../rendering/Sprites.hpp"

#include <glm/glm.hpp>

class Splitter : public Entity, public IInteractable, public IInventoryHolder, public IItemPort, public IBreakable {
public:
    Splitter();
    ~Splitter() = default;

    // Entity
    void update(FrameInput const &input, float elapsed, World const &world) override;
	auto hitbox() const -> Rect override;
    void draw(Sprites &batch) const override;
    void get_occupied(std::vector<glm::ivec2> *ans) const override;

    std::string save() const override;
    void load(std::string line) override;

    // IInteractable
    void on_interact(Context &ctx) override;

    // IInventoryHolder
    auto get_inventory() -> Inventory* override;

    // IItemPort
    PortType get_port(glm::ivec2 grid_coord) override;
    int push_item(ItemRecord item_record) override;

    // IBreakable
    int get_hardness() override {return 1;}
    item_id_t get_drop_id() override;

    // Splitter
    void draw_hint(Sprites &batch) const;

    enum class WorkMode {
        SPLIT = 0,
        JOIN,
        CROSSOVER,
    } work_mode = WorkMode::SPLIT;

    enum class Rotation {
        EAST = 0,
        NORTH,
        WEST,
        SOUTH
    } rotation = Rotation::EAST;


private:
    Inventory inventory;
    size_t frame;
    size_t base_frame;
    
    const float PUSH_COOLDOWN = 0.2f;
    float push_timer = 0.0f;
    Rotation output_poll = Rotation::EAST;
    glm::ivec2 last_push_query = glm::ivec2{};
};