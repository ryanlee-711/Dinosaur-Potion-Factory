#include "Dragon.hpp"
#include "../World.hpp"
#include "../TileMap.hpp"
#include "../../starter/Load.hpp"
#include "../../starter/data_path.hpp"

#include <cmath>


Dragon::Dragon() : inventory(3) {
    inventory.set_rule(0, Inventory::SlotRule{ .required_tags = 0u });
    inventory.set_rule(1, Inventory::SlotRule{ .required_tags = 0u });
    inventory.set_rule(2, Inventory::SlotRule{ .can_insert = false });
}


void Dragon::update(FrameInput const &input, float elapsed, World const &world) {

    // Dragon Update:
    // 1. Verify which recipe to use. If current_recipe is empty or no longer have enough items,
    //    find the new recipe from the registry (and reset timer).
    // 2. Decrement timer accordingly.
    // 3. If timer timeouts, add that new item to matching output slots.

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
        auto recipes = Recipe::get_recipes("dragon");
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

    // step 3. if processing timer timeouts, brew
    if (current_recipe != nullptr && processing_timer <= 0.0f) {
        processing_timer = current_recipe->time_cost;
        for (ItemRecord const &item : current_recipe->items_in) {
            inventory.remove(item.first, item.second);
        }
        // check that there is space for outputs
        for (ItemRecord const &item : current_recipe->items_out) {
            inventory.add_to_slot_range(item.first, item.second, 2, 3);
            if (world.made_items) world.made_items->insert(item.first);
        }
    }
}

void Dragon::draw(Sprites &batch) const {

}

auto Dragon::hitbox() const -> Rect {
    return Rect{position.x, position.y - 32.0f, 128.0f, 48.0f};
}

void Dragon::get_occupied(std::vector<glm::ivec2> *ans) const {
    auto tile = Tilemap::tile_at_point(position + glm::vec2{GameConfig::TILE_SIZE / 2.0f});
    for (int y = 0; y < 3; ++y) {
        for (int x = 0; x < 8; ++x) {
            ans->emplace_back(tile + glm::ivec2{x, -y});   
        }
    }
}

void Dragon::on_interact(Context &ctx) {
    // should trigger inventory display through some mechanism
    ctx.open_container = &inventory;
}

auto Dragon::get_inventory() -> Inventory* {
    return &inventory;
}


float Dragon::get_progress() const {
    if (current_recipe == nullptr || current_recipe->time_cost <= 0.0f) return 0.0f;
    return std::clamp(1.0f - processing_timer / current_recipe->time_cost, 0.0f, 1.0f);
}