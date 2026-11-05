#include "Cauldron.hpp"
#include "../World.hpp"
#include "../TileMap.hpp"
#include "../../starter/Load.hpp"
#include "../../starter/data_path.hpp"

#include <cmath>

static Load< Texture > box_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/sprites/machines/cauldron.png"));
});


Cauldron::Cauldron() : inventory(3) {
    inventory.set_rule(0, Inventory::SlotRule{ .required_tags = ItemTag::Liquid | ItemTag::Potion });
    inventory.set_rule(1, Inventory::SlotRule{ .required_tags = ItemTag::Brewable });
    inventory.set_rule(2, Inventory::SlotRule{ .can_insert = false });
}

void Cauldron::update(FrameInput const &input, float elapsed, World const &world) {

    // animation
    anim_timer += elapsed;
    if (anim_timer > 8.0f) anim_timer -= 8.0f;

    // Cauldron Update:
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
        auto recipes = Recipe::get_recipes("brewer");
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

    auto source_tile = origin + glm::ivec2(1, 0);
    auto target_tile = origin + glm::ivec2{2, 0};

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

void Cauldron::draw(Sprites &batch) const {
    int frame = static_cast<int>(anim_timer * 8.0f) % 4;
    Rect src{64.0f * frame, 0.0f, 64.0f, 64.0f};
    //round to whole pixels so the sprite never sits between pixels:
    glm::vec2 top_left = glm::round(position);
    batch.draw(*box_texture, src, top_left - glm::vec2(0.0f, 48.0f));

}

auto Cauldron::hitbox() const -> Rect {
    return Rect{position.x, position.y - 16.0f, 64.0f, 32.0f};
}

void Cauldron::get_occupied(std::vector<glm::ivec2> *ans) const {
    auto tile = Tilemap::tile_at_point(position + glm::vec2{GameConfig::TILE_SIZE / 2.0f});
    for (int i = 0; i < 8; ++i) ans->emplace_back(tile + glm::ivec2{i % 4, -i / 4});
}

void Cauldron::on_interact(Context &ctx) {
    // should trigger inventory display through some mechanism
    ctx.open_container = &inventory;
}

auto Cauldron::get_inventory() -> Inventory* {
    return &inventory;
}

IItemPort::PortType Cauldron::get_port(glm::ivec2 grid_coord) {
    auto tile = Tilemap::tile_at_point(position + glm::vec2{GameConfig::TILE_SIZE / 2.0f});
    if (grid_coord == tile + glm::ivec2{-1, -1}) {
        return IItemPort::PortType::In;
    }
    if (grid_coord == tile + glm::ivec2{1, 2}) {
        return IItemPort::PortType::In;
    }
    if (grid_coord == tile + glm::ivec2{1, -1}) {
        return IItemPort::PortType::In;
    }
    if (grid_coord == tile + glm::ivec2{4, -1}) {
        return IItemPort::PortType::Out;
    }
    return IItemPort::PortType::None;
}

int Cauldron::push_item(ItemRecord item_record) {

    if (item_record.second <= 0) {
        return 0;
    }

    // goes into whichever input slot accepts it
    for (size_t slot = 0; slot < 2; ++slot) {
        if (!inventory.can_insert(slot, item_record.first)) continue;
        int moved = inventory.insert_into_slot(slot, item_record.first, 1);
        if (moved > 0) return moved;
    }
    return 0;
}

float Cauldron::get_progress() const {
    if (current_recipe == nullptr || current_recipe->time_cost <= 0.0f) return 0.0f;
    return std::clamp(1.0f - processing_timer / current_recipe->time_cost, 0.0f, 1.0f);
}