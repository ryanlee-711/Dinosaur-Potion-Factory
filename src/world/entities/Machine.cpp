#include "Machine.hpp"
#include "../World.hpp"
#include "../TileMap.hpp"
#include "../../main/Rect.hpp"
#include "../../starter/Load.hpp"
#include "../../starter/data_path.hpp"
#include "../../rendering/Particles.hpp"
#include <algorithm>
#include <cmath>

#include <sstream>
#include <random>



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

static Load< Texture > distiller_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/sprites/machines/distiller.png"));
});

static Load< Texture > fermenter_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/sprites/machines/fermenter.png"));
});

static Load< Texture > fuelbox_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/sprites/machines/golem_dropbox.png"));
});

static Load< Texture > siphon_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/sprites/machines/siphon.png"));
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
            /* hitbox_rot_rect= */ Rect{32.0f, -48.0f, 32.0f, 64.0f},
            /* draw_offset= */ glm::vec2{0.0f, -48.0f},
            /* output_slot_begin= */ 2,
            /* output_slot_end= */ 3,
            /* num_sprite_rows= */ 4,
            /* num_sprite_columns= */ 4,
            /* port_locs_in = */ MachineDef::PortLocs{
                std::vector<MachineDef::PortPair>{{ivec2{0, -1}, ivec2{-1, -1}}, {ivec2{2, 0}, ivec2{2, 1}}},
                std::vector<MachineDef::PortPair>{{ivec2{2, -1}, ivec2{1, -1}}, {ivec2{3, 0}, ivec2{3, 1}}},
                std::vector<MachineDef::PortPair>{{ivec2{3, -1}, ivec2{4, -1}}, {ivec2{1, 0}, ivec2{1, 1}}},
                std::vector<MachineDef::PortPair>{{ivec2{2, -3}, ivec2{2, -4}}, {ivec2{3, -1}, ivec2{4, -1}}},
            },
            /* port_locs_out = */ MachineDef::PortLocs{
                std::vector<MachineDef::PortPair>{{ivec2{3, -1}, ivec2{4, -1}}},
                std::vector<MachineDef::PortPair>{{ivec2{2, -3}, ivec2{2, -4}}},
                std::vector<MachineDef::PortPair>{{ivec2{0, -1}, ivec2{-1, -1}}},
                std::vector<MachineDef::PortPair>{{ivec2{3, 0}, ivec2{3, 1}}},
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
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{3, -1}, ivec2{4, -1}}},
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{3, -1}, ivec2{4, -1}}},
            },
            /* port_locs_out = */ MachineDef::PortLocs{
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{3, 0}, ivec2{4, 0}}},
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{3, 0}, ivec2{4, 0}}},
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{0, 0}, ivec2{-1, 0}}},
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{0, 0}, ivec2{-1, 0}}},
            }
        });

        machines->emplace("furnace", MachineDef{
            /* machine_type= */ "furnace",
            /* texture= */ furnace_texture,
            /* draw_rect= */ Rect{0.0f, 0.0f, 64.0f, 64.0f},
            /* hitbox_rect= */ Rect{0.0f, -16.0f, 64.0f, 32.0f},
            /* hitbox_rot_rect= */ Rect{0.0f, -16.0f, 64.0f, 32.0f},
            /* draw_offset= */ glm::vec2{0.0f, -48.0f},
            /* output_slot_begin= */ 1,
            /* output_slot_end= */ 2,
            /* num_sprite_rows= */ 4,
            /* num_sprite_columns= */ 4,
            /* port_locs_in = */ MachineDef::PortLocs{
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{0, -1}, ivec2{-1, -1}}},
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{0, -1}, ivec2{-1, -1}}},
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{3, -1}, ivec2{4, -1}}},
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{3, -1}, ivec2{4, -1}}},
            },
            /* port_locs_out = */ MachineDef::PortLocs{
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{3, -1}, ivec2{4, -1}}},
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{3, -1}, ivec2{4, -1}}},
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{0, -1}, ivec2{-1, -1}}},
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{0, -1}, ivec2{-1, -1}}},
            }
        });

        machines->emplace("chest", MachineDef{
            /* machine_type= */ "chest",
            /* texture= */ chest_texture,
            /* draw_rect= */ Rect{0.0f, 0.0f, 32.0f, 16.0f},
            /* hitbox_rect= */ Rect{0.0f, 0.0f, 32.0f, 16.0f},
            /* hitbox_rot_rect= */ Rect{0.0f, 0.0f, 32.0f, 16.0f},
            /* draw_offset= */ glm::vec2{0.0f, 0.0f},
            /* output_slot_begin= */ 0,
            /* output_slot_end= */ 10,
            /* num_sprite_rows= */ 4,
            /* num_sprite_columns= */ 4,
            /* port_locs_in = */ MachineDef::PortLocs{
                std::vector<MachineDef::PortPair>{{ivec2{0, 0}, ivec2{-1, 0}}},
                std::vector<MachineDef::PortPair>{{ivec2{1, 0}, ivec2{2, 0}}},
                std::vector<MachineDef::PortPair>{{ivec2{0, 0}, ivec2{-1, 0}}},
                std::vector<MachineDef::PortPair>{{ivec2{1, 0}, ivec2{2, 0}}},
            },
            /* port_locs_out = */ MachineDef::PortLocs{
                std::vector<MachineDef::PortPair>{{ivec2{1, 0}, ivec2{2, 0}}},
                std::vector<MachineDef::PortPair>{{ivec2{0, 0}, ivec2{-1, 0}}},
                std::vector<MachineDef::PortPair>{{ivec2{1, 0}, ivec2{2, 0}}},
                std::vector<MachineDef::PortPair>{{ivec2{0, 0}, ivec2{-1, 0}}},
            },
            /* storage = */ true
        });

        machines->emplace("distiller", MachineDef{
            /* machine_type= */ "distiller",
            /* texture= */ distiller_texture,
            /* draw_rect= */ Rect{0.0f, 0.0f, 48.0f, 64.0f},
            /* hitbox_rect= */ Rect{0.0f, -16.0f, 48.0f, 32.0f},
            /* hitbox_rot_rect= */ Rect{0.0f, -16.0f, 48.0f, 32.0f},
            /* draw_offset= */ glm::vec2{0.0f, -48.0f},
            /* output_slot_begin= */ 1,
            /* output_slot_end= */ 3,
            /* num_sprite_rows= */ 1,
            /* num_sprite_columns= */ 4,
            /* port_locs_in = */ MachineDef::PortLocs{
                std::vector<MachineDef::PortPair>{{ivec2{0, -1}, ivec2{-1, -1}}, {ivec2{1, 0}, ivec2{1, 1}}},
                std::vector<MachineDef::PortPair>{{ivec2{0, -1}, ivec2{-1, -1}}, {ivec2{1, 0}, ivec2{1, 1}}},
                std::vector<MachineDef::PortPair>{{ivec2{0, -1}, ivec2{-1, -1}}, {ivec2{1, 0}, ivec2{1, 1}}},
                std::vector<MachineDef::PortPair>{{ivec2{0, -1}, ivec2{-1, -1}}, {ivec2{1, 0}, ivec2{1, 1}}},
            },
            /* port_locs_out = */ MachineDef::PortLocs{
                std::vector<MachineDef::PortPair>{{ivec2{2, -1}, ivec2{3, -1}}},
                std::vector<MachineDef::PortPair>{{ivec2{2, -1}, ivec2{3, -1}}},
                std::vector<MachineDef::PortPair>{{ivec2{2, -1}, ivec2{3, -1}}},
                std::vector<MachineDef::PortPair>{{ivec2{2, -1}, ivec2{3, -1}}},
            }
        });

        machines->emplace("fermenter", MachineDef{
            /* machine_type= */ "fermenter",
            /* texture= */ fermenter_texture,
            /* draw_rect= */ Rect{0.0f, 0.0f, 64.0f, 64.0f},
            /* hitbox_rect= */ Rect{0.0f, -16.0f, 64.0f, 32.0f},
            /* hitbox_rot_rect= */ Rect{0.0f, -16.0f, 64.0f, 32.0f},
            /* draw_offset= */ glm::vec2{0.0f, -48.0f},
            /* output_slot_begin= */ 2,
            /* output_slot_end= */ 4,
            /* num_sprite_rows= */ 4,
            /* num_sprite_columns= */ 1,
            /* port_locs_in = */ MachineDef::PortLocs{
                std::vector<MachineDef::PortPair>{{ivec2{0, -1}, ivec2{-1, -1}}, {ivec2{0, 0}, ivec2{0, 1}}},
                std::vector<MachineDef::PortPair>{{ivec2{0, -1}, ivec2{-1, -1}}, {ivec2{0, 0}, ivec2{0, 1}}},
                std::vector<MachineDef::PortPair>{{ivec2{3, -1}, ivec2{4, -1}}, {ivec2{3, 0}, ivec2{3, 1}}},
                std::vector<MachineDef::PortPair>{{ivec2{3, -1}, ivec2{4, -1}}, {ivec2{3, 0}, ivec2{3, 1}}},
            },
            /* port_locs_out = */ MachineDef::PortLocs{
                std::vector<MachineDef::PortPair>{{ivec2{3, 0}, ivec2{4, 0}}},
                std::vector<MachineDef::PortPair>{{ivec2{3, 0}, ivec2{4, 0}}},
                std::vector<MachineDef::PortPair>{{ivec2{0, 0}, ivec2{-1, 0}}},
                std::vector<MachineDef::PortPair>{{ivec2{0, 0}, ivec2{-1, 0}}},
            }
        });

        machines->emplace("siphon", MachineDef{
            /* machine_type= */ "siphon",
            /* texture= */ siphon_texture,
            /* draw_rect= */ Rect{0.0f, 0.0f, 64.0f, 64.0f},
            /* hitbox_rect= */ Rect{0.0f, -48.0f, 64.0f, 64.0f},
            /* hitbox_rot_rect= */ Rect{0.0f, -48.0f, 64.0f, 64.0f},
            /* draw_offset= */ glm::vec2{0.0f, -48.0f},
            /* output_slot_begin= */ 1,
            /* output_slot_end= */ 2,
            /* num_sprite_rows= */ 4,
            /* num_sprite_columns= */ 1,
            /* port_locs_in = */ MachineDef::PortLocs{
                std::vector<MachineDef::PortPair>{{ivec2{3, -2}, ivec2{4, -2}}},
                std::vector<MachineDef::PortPair>{{ivec2{0, -2}, ivec2{-1, -2}}},
                std::vector<MachineDef::PortPair>{{ivec2{0, -2}, ivec2{-1, -2}}},
                std::vector<MachineDef::PortPair>{{ivec2{3, -2}, ivec2{4, -2}}},
            },
            /* port_locs_out = */ MachineDef::PortLocs{
                std::vector<MachineDef::PortPair>{{ivec2{1, -3}, ivec2{1, -4}}},
                std::vector<MachineDef::PortPair>{{ivec2{1, 0}, ivec2{1, 1}}},
                std::vector<MachineDef::PortPair>{{ivec2{3, -3}, ivec2{4, -3}}},
                std::vector<MachineDef::PortPair>{{ivec2{0, -3}, ivec2{-1, -3}}},
            }
        });

        machines->emplace("altar", MachineDef{
            /* machine_type= */ "altar",
            /* texture= */ fermenter_texture,
            /* draw_rect= */ Rect{0.0f, 0.0f, 48.0f, 64.0f},
            /* hitbox_rect= */ Rect{0.0f, -32.0f, 48.0f, 48.0f},
            /* hitbox_rot_rect= */ Rect{0.0f, -32.0f, 48.0f, 48.0f},
            /* draw_offset= */ glm::vec2{0.0f, -48.0f},
            /* output_slot_begin= */ 2,
            /* output_slot_end= */ 4,
            /* num_sprite_rows= */ 4,
            /* num_sprite_columns= */ 4,
            /* port_locs_in = */ MachineDef::PortLocs{
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{0, -1}, ivec2{-1, -1}}},
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{0, -1}, ivec2{-1, -1}}},
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{2, -1}, ivec2{3, -1}}},
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{2, -1}, ivec2{3, -1}}},
            },
            /* port_locs_out = */ MachineDef::PortLocs{
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{2, -1}, ivec2{3, -1}}},
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{2, -1}, ivec2{3, -1}}},
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{0, -1}, ivec2{-1, -1}}},
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{0, -1}, ivec2{-1, -1}}},
            }
        });

        machines->emplace("fuelbox", MachineDef{
            /* machine_type= */ "fuelbox",
            /* texture= */ fuelbox_texture,
            /* draw_rect= */ Rect{0.0f, 0.0f, 32.0f, 16.0f},
            /* hitbox_rect= */ Rect{0.0f, 0.0f, 32.0f, 16.0f},
            /* hitbox_rot_rect= */ Rect{0.0f, 0.0f, 32.0f, 16.0f},
            /* draw_offset= */ glm::vec2{0.0f, 0.0f},
            /* output_slot_begin= */ 1,
            /* output_slot_end= */ 2,
            /* num_sprite_rows= */ 4,
            /* num_sprite_columns= */ 4,
            /* port_locs_in = */ MachineDef::PortLocs{
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{0, 0}, ivec2{-1, 0}}, MachineDef::PortPair{ivec2{1, 0}, ivec2{2, 0}}},
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{0, 0}, ivec2{-1, 0}}, MachineDef::PortPair{ivec2{1, 0}, ivec2{2, 0}}},
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{0, 0}, ivec2{-1, 0}}, MachineDef::PortPair{ivec2{1, 0}, ivec2{2, 0}}},
                std::vector<MachineDef::PortPair>{MachineDef::PortPair{ivec2{0, 0}, ivec2{-1, 0}}, MachineDef::PortPair{ivec2{1, 0}, ivec2{2, 0}}},
            },
            /* port_locs_out = */ MachineDef::PortLocs{
                std::vector<MachineDef::PortPair>{},
                std::vector<MachineDef::PortPair>{},
                std::vector<MachineDef::PortPair>{},
                std::vector<MachineDef::PortPair>{},
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

Machine::Machine(size_t inventory_size, item_id_t machine_type, int rot) : inventory(inventory_size) {
    rotation = static_cast<Rotation>(rot % 4);
    if (def = MachineDef::get(machine_type); def == nullptr) {
        throw std::runtime_error("Machine Not implemented! " + machine_type);
    }
    configure_inventory(machine_type);
}

void Machine::configure_inventory(item_id_t machine_type) {
    if (machine_type == "brewer") {
        inventory.set_rule(0, Inventory::SlotRule{ .required_tags = ItemTag::Brewable });
        inventory.set_rule(1, Inventory::SlotRule{ .required_tags = ItemTag::Liquid | ItemTag::Potion });
        inventory.set_rule(2, Inventory::SlotRule{ .can_insert = false });
    } else if (machine_type == "crusher") {
        inventory.set_rule(0, Inventory::SlotRule{ .required_tags = ItemTag::Crushable });
        inventory.set_rule(1, Inventory::SlotRule{ .can_insert = false });
    } else if (machine_type == "furnace") {
        inventory.set_rule(0, Inventory::SlotRule{ .required_tags = ItemTag::Burnable });
        inventory.set_rule(1, Inventory::SlotRule{ .can_insert = false });
    } else if (machine_type == "altar") {
        inventory.set_rule(1, Inventory::SlotRule{ .can_insert = false });
    } else if (machine_type == "siphon") {
        inventory.set_rule(1, Inventory::SlotRule{ .can_insert = false });
    } else if (machine_type == "chest") {
        // pass
    } else if (machine_type == "distiller") {
        inventory.set_rule(0, Inventory::SlotRule{ .required_tags = ItemTag::Liquid | ItemTag::Potion });
        inventory.set_rule(1, Inventory::SlotRule{ .can_insert = false });
        inventory.set_rule(2, Inventory::SlotRule{ .can_insert = false, .required_tags = ItemTag::Liquid | ItemTag::Potion });
    } else if (machine_type == "fermenter") {
        inventory.set_rule(0, Inventory::SlotRule{ .required_tags = ItemTag::Fermentable });
        inventory.set_rule(1, Inventory::SlotRule{ .required_tags = ItemTag::Liquid | ItemTag::Potion });
        inventory.set_rule(2, Inventory::SlotRule{ .can_insert = false });
        inventory.set_rule(3, Inventory::SlotRule{ .can_insert = false, .required_tags = ItemTag::Liquid | ItemTag::Potion });
    } else if (machine_type == "fuelbox") {
        // pass
    } else {
        throw std::runtime_error{"Machine Not Implemented: " + machine_type};
    }
}

ParticlePreset const *Machine::break_hit_particles() const {
    return &Presets::RockChips;
}

void Machine::update(FrameInput const &input, float elapsed, World const &world) {
    shake_timer = std::max(0.0f, shake_timer - elapsed);

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
    if (current_recipe == nullptr && def->machine_type != "altar") {
        recipe_stale = true;
    } else if (def->machine_type != "altar") {
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
    if (current_recipe || def->machine_type == "altar") {
        processing_timer -= elapsed;
    }

    // step 2.5. roll RNG
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    float dice = dist(world.rng);

    // step 3. if processing timer timeouts, produce
    if (current_recipe != nullptr && processing_timer <= 0.0f) {
        processing_timer = current_recipe->time_cost;
        for (size_t idx = 0; idx < current_recipe->items_in.size(); ++idx) {
            if (dice > current_recipe->probability && (def->machine_type == "fermenter" && idx == 0)) continue;
            ItemRecord const &item = current_recipe->items_in[idx];
            inventory.remove(item.first, item.second);
        }
        // check that there is space for outputs
        for (size_t idx = 0; idx < current_recipe->items_out.size(); ++idx) {
            if (dice > current_recipe->probability &&
                ((def->machine_type == "distiller" && idx == 0) || (def->machine_type == "fermenter" && idx == 0))) continue;
            ItemRecord const &item = current_recipe->items_out[idx];
            inventory.add_to_slot_range(item.first, item.second, def->output_slot_begin, def->output_slot_end);
            if (world.made_items) world.made_items->insert(item.first);
        }
        current_recipe = nullptr;
    } else if (def->machine_type == "altar") {
        //whatever is offered disappears; potions leave their empty bottles behind
        item_id_t offered = inventory.get_slot_id(0);
        if (offered != "") {
            Item const *item = Item::get(offered);
            if (item != nullptr && item->has_tag(ItemTag::Potion)) {
                inventory.add_to_slot_range("empty_bottle", inventory[0].second, 1, 2);
            }
            inventory.set_slot(0, "", 0);
        }
    }

    // step 4. Item pushing: every push_timer cooldown, send one output item through
    // whichever output port (from this machine's MachineDef) has something that accepts it.
    push_timer -= elapsed;
    if (push_timer > 0.0f) return;
    push_timer = 0.0f;

    auto origin = Tilemap::tile_at_point(position + glm::vec2{GameConfig::TILE_SIZE / 2.0f});
    size_t out_begin = def->storage ? 0 : def->output_slot_begin;
    size_t out_end = def->storage ? inventory.size() : std::min(def->output_slot_end, inventory.size());

    for (auto const &[src, dst] : def->port_locs_out[rot2idx(rotation)]) {
        Entity *target = world.entity_at(origin + dst);
        if (!target || target == this) continue;

        auto *port = dynamic_cast<IItemPort*>(target);
        if (!port) continue;

        auto type = port->get_port(origin + src);
        if (type != PortType::In && type != PortType::Both) continue;

        for (size_t slot = out_begin; slot < out_end; ++slot) {
            if (inventory.slot_empty(slot)) continue;
            int sent = port->push_item({inventory.get_slot_id(int(slot)), 1});
            if (sent <= 0) continue;
            inventory.extract_from_slot(slot, sent);
            push_timer = PUSH_COOLDOWN;
            return;
        }
    }
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
    if (shake_timer > 0.0f) top_left.x += (std::fmod(shake_timer, 0.06f) < 0.03f) ? 1.0f : -1.0f; //wobble from a pickaxe hit
    // Some sheets only provide one orientation; reuse it for every rotation.
    size_t row = rot2idx(rotation) % def->num_sprite_rows;
    batch.draw(*def->texture, Rect{def->texture_rect.w * frame, def->texture_rect.h * row, def->texture_rect.w, def->texture_rect.h}, top_left);

}

void Machine::draw_hint(Sprites &batch, Load<Texture> &tex, float timer) const {
    // draw the hints for this Machine
    if (!def) return;

    constexpr float TILESIZE = 16.0f;
    constexpr float GRIDSIZE = 18.0f;
    constexpr float OFFSET = 2.0f;

    auto port_pair_to_texrect = [](glm::ivec2 dir, int md = 2) -> Rect {
        if (dir.x == 1 && dir.y == 0) {
            return Rect{GRIDSIZE * 0 + OFFSET, GRIDSIZE * md + OFFSET, TILESIZE, TILESIZE};
        }
        if (dir.x == 0 && dir.y == -1) {
            return Rect{GRIDSIZE * 1 + OFFSET, GRIDSIZE * md + OFFSET, TILESIZE, TILESIZE};
        }
        if (dir.x == -1 && dir.y == 0) {
            return Rect{GRIDSIZE * 2 + OFFSET, GRIDSIZE * md + OFFSET, TILESIZE, TILESIZE};
        }
        if (dir.x == 0 && dir.y == 1) {
            return Rect{GRIDSIZE * 3 + OFFSET, GRIDSIZE * md + OFFSET, TILESIZE, TILESIZE};
        }
        return Rect{0.0f, 5.0f, 1.0f, 1.0f};
    };

    glm::vec2 top_left = glm::round(position);
    for (auto const &[src, dst] : def->port_locs_in[rot2idx(rotation)]) {
        Rect rect = port_pair_to_texrect(src - dst);
        glm::vec2 target_pos = top_left + glm::vec2(src) * TILESIZE;
        batch.draw(*tex, rect, target_pos, glm::u8vec4(0x40, 0x80, 0xff, 0xa0));
    }
    for (auto const &[src, dst] : def->port_locs_out[rot2idx(rotation)]) {
        Rect rect = port_pair_to_texrect(dst - src, 3);
        glm::vec2 target_pos = top_left + glm::vec2(src) * TILESIZE;
        batch.draw(*tex, rect, target_pos, glm::u8vec4(0x40, 0x80, 0xff, 0xa0));
    }
}


auto Machine::hitbox() const -> Rect {
    Rect const &rect = rot2idx(rotation) % 2 ? def->hitbox_rot_rect : def->hitbox_rect;
    return rect.offset_by(position);
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

    // goes into whichever input slot accepts it (a chest: any slot)
    size_t in_end = def->storage ? inventory.size() : def->output_slot_begin;
    for (size_t slot = 0; slot < in_end; ++slot) {
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



std::string Machine::save() const {
    std::ostringstream ss;
    ss << uid;
    ss << ' ' << "machine";
    ss << ' ' << def->machine_type;
    ss << ' ' << position.x << ' ' << position.y << ' ' << rot2idx(rotation);
    for (int idx = 0; idx < int(inventory.size()); ++idx) {
        item_id_t item_id = inventory.get_slot_id(idx);
        if (item_id == "") item_id = "null";
        int count = inventory.get_slot_count(idx);
        ss << ' ' << item_id << ' ' << count;
    }
    return ss.str();
}

void Machine::load(std::string line) {
    std::istringstream ss(line);

    /**
     * Store Format:
     *   <uid> <pos.x> <pos.y> <rotation_idx> [Inventory]
     */
    uint32_t read_uid;
    std::string entity_type, machine_type;

    if (!(ss >> read_uid >> entity_type >> machine_type)) {
        throw std::runtime_error{"UID read failure"};
    }
    if (read_uid != uid && uid != 0) {
        throw std::runtime_error{"Loaded data mismatches the entity's UID!"};
    }

    int rot_idx = 0;
    if (!(ss >> position.x >> position.y >> rot_idx)) {
        throw std::runtime_error{"Didn't read position/rotation"};
    }
    if (rot_idx < 0 || rot_idx > 3) {
        throw std::runtime_error{"Invalid Rotation"};
    }
    rotation = static_cast<Rotation>(rot_idx);

    for (int idx = 0; idx < int(inventory.size()); ++idx) {
        item_id_t item_id;
        int count;
        if (!(ss >> item_id >> count)) {
            throw std::runtime_error{"Invalid inventory item_id / count while loading"};
        }
        if (item_id == "null") item_id = "";
        inventory.set_slot(idx, item_id, count);
    }
}
