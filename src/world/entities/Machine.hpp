#pragma once

#include "../Entity.hpp"
#include "../../inventory/Item.hpp"
#include "../Components.hpp"
#include "../../main/Rect.hpp"
#include "../../rendering/Sprites.hpp"
#include "../../starter/Load.hpp"
#include "../../inventory/Recipe.hpp"

#include <array>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <glm/glm.hpp>

struct MachineDef {

    using MachineRegistry = std::unordered_map<item_id_t, MachineDef>;

    item_id_t machine_type = "";
    Load<Texture> &texture;
    Rect texture_rect;
    Rect hitbox_rect;
    Rect hitbox_rot_rect;
    glm::vec2 draw_offset;

    // Machine-specific definitions. Each type of machine shares the same values, populated in factorizing stage
    size_t output_slot_begin = 0;
    size_t output_slot_end = 0;
    // Machine animations.
    // Each spritesheet has four rows (EAST, NORTH, WEST, SOUTH)
    // Each row has as many frames as needed. All share the same animator
    size_t num_sprite_rows = 4;
    size_t num_sprite_columns = 1;

    // Item ports definition. It is easier to hardcode than write rotation functions
    using PortPair = std::pair<glm::ivec2, glm::ivec2>;
    using PortLocs = std::array<std::vector<PortPair>, 4>;
    PortLocs port_locs_in;
    PortLocs port_locs_out;
    bool storage = false;

    // getter
    static const MachineDef* get(item_id_t machine_id);
};



class Machine : public Entity, public IInteractable, public IInventoryHolder, public IItemPort, public IBreakable, public IProgress {
    /* General purpose machine implementation, shared by all machines.
     */
public:
    Machine();
    Machine(size_t inventory_size, item_id_t machine_type, int rot = 0);
    ~Machine() = default;

    void configure_inventory(item_id_t machine_type);

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
    float get_progress() const override;

    // IBreakable
    item_id_t get_drop_id() override {return def ? def->machine_type : "";}
    void on_break_hit() override { shake_timer = SHAKE_SECONDS; }    // wobble on each pickaxe hit
    ParticlePreset const *break_hit_particles() const override;    // rock chips on each pickaxe hit

    // Machine-specific
    enum Rotation {EAST, NORTH, WEST, SOUTH};
    Rotation const get_rotation() {return rotation;}
    static inline size_t rot2idx(Rotation const &rot) {
        return static_cast<size_t>(rot);
    }

    void draw_hint(Sprites &batch, Load<Texture> &tex, float timer) const;
    item_id_t get_machine_type() {return def->machine_type;};

    // These may not change once placed.
    Rotation rotation = Rotation::EAST;

private:
    MachineDef const *def = nullptr;

    static constexpr float SHAKE_SECONDS = 0.15f; // how long one pickaxe hit wobbles it
    float shake_timer = 0.0f;                     // > 0: wobbling from a pickaxe hit

    // ===== Dynamic Machine states =====
    Inventory inventory;
    float push_timer = 0.0f;
    float processing_timer = 0.0f;
    float anim_timer = 0.0f;

    const Recipe *current_recipe = nullptr;
};