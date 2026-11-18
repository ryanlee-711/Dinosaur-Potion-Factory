#include "Machine.hpp"
#include "../World.hpp"
#include "../TileMap.hpp"
#include "../../main/Rect.hpp"
#include "../../starter/Load.hpp"
#include "../../starter/data_path.hpp"
#include <cmath>



#include <iostream>
static Load< Texture > brewer_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/sprites/machines/cauldron.png"));
});

static Load< Texture > crusher_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/sprites/machines/crusher.png"));
});

static Load< Texture > furnace_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/sprites/machines/furnace.png"));
});

static Load< Texture > chest_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/chest.png"));
});

// MACHINE DEFINITIONS
using ivec2 = glm::ivec2;
using MachineRegistry = MachineDef::MachineRegistry;

static Load<MachineRegistry> machine_registry(LoadTagDefault,
	[]() -> MachineRegistry const* {
		auto machines = std::make_unique<MachineRegistry>();

		machines->emplace("brewer", MachineDef{
            /* machine_type= */ "brewer",
            /* texture= */ brewer_texture,
            /* draw_rect= */ Rect{0.0f, 0.0f, 64.0f, 64.0f},
            /* hitbox_rect= */ Rect{0.0f, -16.0f, 64.0f, 32.0f},
            /* hitbox_rot_rect= */ Rect{0.0f, -48.0f, 32.0f, 64.0f},
            /* draw_offset= */ glm::vec2{0.0f, -48.0f},
            /* output_slot_begin= */ 2,
            /* output_slot_end= */ 3,
            /* num_sprite_rows= */ 4,
            /* num_sprite_columns= */ 4,
            /* port_locs_in = */ MachineDef::PortLocs{
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{0, -1}, ivec2{-1, -1}}, MachineDef::PortPair{ivec2{1, 0}, ivec2{1, 1}}},
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{0, -1}, ivec2{-1, -1}}, MachineDef::PortPair{ivec2{1, 0}, ivec2{1, 1}}},
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{0, -1}, ivec2{-1, -1}}, MachineDef::PortPair{ivec2{1, 0}, ivec2{1, 1}}},
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{0, -1}, ivec2{-1, -1}}, MachineDef::PortPair{ivec2{1, 0}, ivec2{1, 1}}},
            },
            /* port_locs_out = */ MachineDef::PortLocs{
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{3, -1}, ivec2{4, -1}}},
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{3, -1}, ivec2{4, -1}}},
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{3, -1}, ivec2{4, -1}}},
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{3, -1}, ivec2{4, -1}}},
            }
        });

        machines->emplace("crusher", MachineDef{
            /* machine_type= */ "crusher",
            /* texture= */ crusher_texture,
            /* draw_rect= */ Rect{0.0f, 0.0f, 64.0f, 64.0f},
            /* hitbox_rect= */ Rect{0.0f, -16.0f, 64.0f, 32.0f},
            /* hitbox_rot_rect= */ Rect{0.0f, -48.0f, 32.0f, 64.0f},
            /* draw_offset= */ glm::vec2{0.0f, -48.0f},
            /* output_slot_begin= */ 1,
            /* output_slot_end= */ 2,
            /* num_sprite_rows= */ 4,
            /* num_sprite_columns= */ 4,
            /* port_locs_in = */ MachineDef::PortLocs{
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{0, -1}, ivec2{-1, -1}}},
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{0, -1}, ivec2{-1, -1}}},
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{0, -1}, ivec2{-1, -1}}},
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{0, -1}, ivec2{-1, -1}}},
            },
            /* port_locs_out = */ MachineDef::PortLocs{
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{3, -1}, ivec2{4, -1}}},
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{3, -1}, ivec2{4, -1}}},
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{3, -1}, ivec2{4, -1}}},
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{3, -1}, ivec2{4, -1}}},
            }
        });

        machines->emplace("furnace", MachineDef{
            /* machine_type= */ "furnace",
            /* texture= */ furnace_texture,
            /* draw_rect= */ Rect{0.0f, 0.0f, 64.0f, 48.0f},
            /* hitbox_rect= */ Rect{0.0f, -16.0f, 64.0f, 32.0f},
            /* hitbox_rot_rect= */ Rect{0.0f, -48.0f, 32.0f, 64.0f},
            /* draw_offset= */ glm::vec2{0.0f, -48.0f},
            /* output_slot_begin= */ 1,
            /* output_slot_end= */ 2,
            /* num_sprite_rows= */ 4,
            /* num_sprite_columns= */ 4,
            /* port_locs_in = */ MachineDef::PortLocs{
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{0, -1}, ivec2{-1, -1}}},
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{0, -1}, ivec2{-1, -1}}},
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{0, -1}, ivec2{-1, -1}}},
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{0, -1}, ivec2{-1, -1}}},
            },
            /* port_locs_out = */ MachineDef::PortLocs{
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{3, -1}, ivec2{4, -1}}},
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{3, -1}, ivec2{4, -1}}},
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{3, -1}, ivec2{4, -1}}},
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{3, -1}, ivec2{4, -1}}},
            }
        });

        machines->emplace("chest", MachineDef{
            /* machine_type= */ "chest",
            /* texture= */ chest_texture,
            /* draw_rect= */ Rect{0.0f, 0.0f, 32.0f, 16.0f},
            /* hitbox_rect= */ Rect{0.0f, 0.0f, 32.0f, 16.0f},
            /* hitbox_rot_rect= */ Rect{0.0f, 0.0f, 32.0f, 16.0f},
            /* draw_offset= */ glm::vec2{0.0f, 0.0f},
            /* output_slot_begin= */ 1,
            /* output_slot_end= */ 2,
            /* num_sprite_rows= */ 4,
            /* num_sprite_columns= */ 4,
            /* port_locs_in = */ MachineDef::PortLocs{
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{0, 0}, ivec2{-1, 0}}},
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{0, 0}, ivec2{-1, 0}}},
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{0, 0}, ivec2{-1, 0}}},
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{0, 0}, ivec2{-1, 0}}},
            },
            /* port_locs_out = */ MachineDef::PortLocs{
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{1, 0}, ivec2{2, 0}}},
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{1, 0}, ivec2{2, 0}}},
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{1, 0}, ivec2{2, 0}}},
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{1, 0}, ivec2{2, 0}}},
            }
        });

		return machines.release();
	}
);

const MachineDef* MachineDef::get(item_id_t machine_id) {
    if (!machine_registry) return nullptr;
	auto it = machine_registry->find(machine_id);
	return it != machine_registry->end() ? &it->second : nullptr;
}

const float PUSH_COOLDOWN = 1.0f;

Machine::Machine() : inventory(3) { }

Machine::Machine(size_t inventory_size, item_id_t machine_type) : inventory(inventory_size) {
    if (def = MachineDef::get(machine_type); def == nullptr) {
        throw std::runtime_error("Not implemented!");
    }
    if (machine_type == "brewer") {
        inventory.set_rule(0, Inventory::SlotRule{ .required_tags = ItemTag::Liquid | ItemTag::Potion });
        inventory.set_rule(1, Inventory::SlotRule{ .required_tags = ItemTag::Brewable });
        inventory.set_rule(2, Inventory::SlotRule{ .can_insert = false });
    } else if (machine_type == "crusher") {
        inventory.set_rule(0, Inventory::SlotRule{ .required_tags = ItemTag::Crushable });
        inventory.set_rule(1, Inventory::SlotRule{ .can_insert = false });
    } else if (machine_type == "furnace") {
        inventory.set_rule(0, Inventory::SlotRule{ .required_tags = ItemTag::Burnable });
        inventory.set_rule(1, Inventory::SlotRule{ .can_insert = false });
    } else if (machine_type == "chest") {
        // pass
    } else {
        throw std::runtime_error{"Not Implemented!"};
    }
}

void Machine::update(FrameInput const &input, float elapsed, World const &world) {
    
    // animation
    if (def->machine_type == "brewer" || processing_timer > 0.0f) {
        anim_timer += elapsed;
    }
    if (anim_timer > 8.0f) anim_timer -= 8.0f;


    // Machine Update:
    // 1. Verify which recipe to use. If current_recipe is empty or no longer have enough items,
    //    find the new recipe from the registry (and reset timer).
    // 2. Decrement timer accordingly.
    // 3. If timer timeouts, add that new item to matching output slots.
    // 4. Do container work (piping outputs)

    // step 1a. verify current_recipe

    bool recipe_stale = false;
    if (current_recipe == nullptr) {
        recipe_stale = true;
    } else {
        for (ItemRecord const &item : current_recipe->items_in) {
            if (inventory.count(item.first) < item.second) {
                recipe_stale = true;
                break;
            }
        }
        for (ItemRecord const &item : current_recipe->items_out) {
            if (inventory.space_for(item.first) < item.second) {
                recipe_stale = true;
                break;
            }
        }
    }
    // if recipe is stale, current_recipe set to null
    if (recipe_stale) {
        current_recipe = nullptr;
        processing_timer = 0.0f;

        // try to find a new recipe
        auto recipes = Recipe::get_recipes(def->machine_type);
        if (recipes != nullptr) for (Recipe const &recipe : *recipes) {
            bool ok = true;
            // check that inputs are sufficient
            for (ItemRecord const &item : recipe.items_in) {
                if (inventory.count(item.first) < item.second) {
                    ok = false;
                    break;
                }
            }
            // check that there is space for outputs
            for (ItemRecord const &item : recipe.items_out) {
                if (inventory.space_for(item.first) < item.second) {
                    ok = false;
                    break;
                }
            }
            // if ok, then set recipe and break
            if (ok) {
                current_recipe = &recipe;
                // also set timer
                processing_timer = current_recipe->time_cost;
                break;
            }
        }
    }

    // step 2. reduce processing_timer
    if (current_recipe) {
        processing_timer -= elapsed;
    }

    // step 3. if processing timer timeouts, produce
    if (current_recipe != nullptr && processing_timer <= 0.0f) {
        processing_timer = current_recipe->time_cost;
        for (ItemRecord const &item : current_recipe->items_in) {
            inventory.remove(item.first, item.second);
        }
        // check that there is space for outputs
        for (ItemRecord const &item : current_recipe->items_out) {
            inventory.add_to_slot_range(item.first, item.second, def->output_slot_begin, def->output_slot_end);
            if (world.made_items) world.made_items->insert(item.first);
        }
    }

    // step 4. Item pushing

    push_timer -= elapsed;
    if (push_timer <= 0.0f) {
        push_timer = 0.0f;
    } else {
        return;
    }

    // push item to the right
    auto origin = Tilemap::tile_at_point(position + glm::vec2{GameConfig::TILE_SIZE / 2.0f});

    auto source_tile = origin + glm::ivec2(3, -1);
    auto target_tile = origin + glm::ivec2{4, -1};

    Entity *target = world.entity_at(target_tile);
    if (!target || target == this) return;

    // get IItemPort!
    auto *port = dynamic_cast<IItemPort*>(target);
    if (!port) return;

    auto type = port->get_port(source_tile);
    if (type != PortType::In && type != PortType::Both) return;

    if (inventory.slots[2].second <= 0) return;
    int item_sent = port->push_item({inventory.slots[2].first, 1});

    if (item_sent != 0) {
        inventory.slots[2].second -= item_sent;
        if (inventory.slots[2].second <= 0) {
            inventory.slots[2].first = "";
            inventory.slots[2].second = 0;
        }
        push_timer = PUSH_COOLDOWN;
    }
    return;
}

void Machine::draw(Sprites &batch) const {
    // When certain machines need special draws, please expand it here by adding if-else.
    // e.g., if (def->machine_type == "brewer") { draw flame lights...}
    int frame = 0;
    if (def->machine_type == "brewer") {
        frame = static_cast<int>(anim_timer * 8.0f) % 4;
    } else if (def->machine_type == "furnace" || def->machine_type == "crusher") {
        constexpr float cooldown = 3.0f;
        frame = 3 - std::clamp(static_cast<int>(std::round(std::abs((processing_timer - cooldown / 2.0f) * 6.0f))), 0, 3);
    } else if (def->machine_type == "chest") {
        frame = 3;
    }

    glm::vec2 top_left = glm::round(position) + def->draw_offset;
    batch.draw(*def->texture, Rect{def->texture_rect.w * frame, def->texture_rect.h * rot2idx(rotation), def->texture_rect.w, def->texture_rect.h}, top_left);

}

auto Machine::hitbox() const -> Rect {
    return def->hitbox_rect.offset_by(position);
}

void Machine::get_occupied(std::vector<glm::ivec2> *ans) const {

    int x = rot2idx(rotation) % 2 ? static_cast<int>(def->hitbox_rot_rect.x) / GameConfig::TILE_SIZE : static_cast<int>(def->hitbox_rect.x) / GameConfig::TILE_SIZE;
    int y = rot2idx(rotation) % 2 ? static_cast<int>(def->hitbox_rot_rect.y) / GameConfig::TILE_SIZE : static_cast<int>(def->hitbox_rect.y) / GameConfig::TILE_SIZE;
    int w = rot2idx(rotation) % 2 ? static_cast<int>(def->hitbox_rot_rect.w) / GameConfig::TILE_SIZE : static_cast<int>(def->hitbox_rect.w) / GameConfig::TILE_SIZE;
    int h = rot2idx(rotation) % 2 ? static_cast<int>(def->hitbox_rot_rect.h) / GameConfig::TILE_SIZE : static_cast<int>(def->hitbox_rect.h) / GameConfig::TILE_SIZE;

    auto tile = Tilemap::tile_at_point(position + glm::vec2{GameConfig::TILE_SIZE / 2.0f});
    for (int dx = 0; dx < w; ++dx) {
        for (int dy = 0; dy < h; ++dy) {
            ans->emplace_back(tile + glm::ivec2{x + dx, y + dy});
        }
    }
}

void Machine::on_interact(Context &ctx) {
    // should trigger inventory display through some mechanism
    ctx.open_container = &inventory;
}

auto Machine::get_inventory() -> Inventory* {
    return &inventory;
}

IItemPort::PortType Machine::get_port(glm::ivec2 grid_coord) {
    auto tile = Tilemap::tile_at_point(position + glm::vec2{GameConfig::TILE_SIZE / 2.0f});

    for (auto const &[src, dst]: def->port_locs_in[rot2idx(rotation)]) {
        if (tile + dst == grid_coord) {
            return IItemPort::PortType::In;
        }
    }

    for (auto const &[src, dst]: def->port_locs_out[rot2idx(rotation)]) {
        if (tile + dst == grid_coord) {
            return IItemPort::PortType::Out;
        }
    }

    return IItemPort::PortType::None;
}

int Machine::push_item(ItemRecord item_record) {
    if (item_record.second <= 0) {
        return 0;
    }

    // goes into whichever input slot accepts it
    for (size_t slot = 0; slot < def->output_slot_begin; ++slot) {
        if (!inventory.can_insert(slot, item_record.first)) continue;
        int moved = inventory.insert_into_slot(slot, item_record.first, 1);
        if (moved > 0) return moved;
    }
    return 0;
}

float Machine::get_progress() const {
    if (current_recipe == nullptr || current_recipe->time_cost <= 0.0f) return 0.0f;
    return std::clamp(1.0f - processing_timer / current_recipe->time_cost, 0.0f, 1.0f);
}