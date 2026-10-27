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

}

void Cauldron::update(FrameInput const &input, float elapsed, World const &world) {
    auto const &entities = world.entities;

    // Processing
    if (inventory.slots[0].second > 0 && inventory.slots[1].second >= 2) {
        processing_timer -= elapsed;
    } else {
        processing_timer = BREW_COOLDOWN;
    }
    
    if (processing_timer <= 0.0f) {
        // Two apple powder + 1 water bottle
        if (inventory.slots[0].first == "water_bottle" && inventory.slots[0].second > 0
            && inventory.slots[1].first != "" && inventory.slots[1].second >= 2
            && (inventory.slots[2].first == "" || inventory.slots[2].first == "ale")) {
            inventory.slots[0].second--;
            inventory.slots[1].second -= 2;

            if (inventory.slots[2].first == "") {
                inventory.slots[2].first = "ale";  // hardcoding the only recipe now
                inventory.slots[2].second = 0;
            }
            inventory.slots[2].second++;
            if (inventory.slots[0].second == 0) {
                inventory.slots[0].first = "";
            }
            if (inventory.slots[1].second == 0) {
                inventory.slots[1].first = "";
            }

            processing_timer = BREW_COOLDOWN;
        }
    }

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

    Entity *target = entity_at(entities, target_tile);
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
    Rect src{0.0f, 0.0f, 32.0f, 80.0f};
    //round to whole pixels so the sprite never sits between pixels:
    glm::vec2 top_left = glm::round(position);
    batch.draw(*box_texture, src, top_left - glm::vec2(0.0f, 64.0f));

}

auto Cauldron::hitbox() const -> Rect {
    return Rect{position.x, position.y, 32.0f, 16.0f};
}

void Cauldron::get_occupied(std::vector<glm::ivec2> *ans) const {
    auto tile = Tilemap::tile_at_point(position + glm::vec2{GameConfig::TILE_SIZE / 2.0f});
	ans->emplace_back(tile);
    ans->emplace_back(tile + glm::ivec2{1, 0});
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
    if (grid_coord == tile + glm::ivec2{-1, 0}) {
        return IItemPort::PortType::In;
    }
    if (grid_coord == tile + glm::ivec2{0, 1}) {
        return IItemPort::PortType::In;
    }
    if (grid_coord == tile + glm::ivec2{0, -1}) {
        return IItemPort::PortType::In;
    }
    if (grid_coord == tile + glm::ivec2{2, 0}) {
        return IItemPort::PortType::Out;
    }
    return IItemPort::PortType::None;
}

int Cauldron::push_item(ItemRecord item_record) {

    if (item_record.second <= 0) {
        return 0;
    }

    if (item_record.first == "water_bottle") {
        return item_record.second - inventory.add_to_slot_range(item_record.first, 1, 0, 1);
    }
    return item_record.second - inventory.add_to_slot_range(item_record.first, 1, 1, 2);
}
