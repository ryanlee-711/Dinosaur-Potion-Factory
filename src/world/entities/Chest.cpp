#include "Chest.hpp"
#include "../World.hpp"
#include "../TileMap.hpp"
#include "../../starter/Load.hpp"
#include "../../starter/data_path.hpp"

#include <cmath>
#include <array>

static Load< Texture > box_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/chest.png"));
});


void Chest::update(FrameInput const &input, float elapsed, World const &world) {
    auto const &entities = world.entities;

    push_timer -= elapsed;
    if (push_timer <= 0.0f) {
        push_timer = 0.0f;
    } else {
        return;
    }

    // push item to the right
    auto origin = Tilemap::tile_at_point(position + glm::vec2{GameConfig::TILE_SIZE / 2.0f});

    auto source_tile = origin + glm::ivec2(1, 0);

    for (auto delta : std::array<glm::ivec2, 3>{glm::ivec2{2, 0}, glm::ivec2{1, 1}, glm::ivec2{1, -1}}) {
        auto target_tile = origin + delta;
        Entity *target = entity_at(entities, target_tile);
        if (!target || target == this) continue;

        // get IItemPort!
        auto *port = dynamic_cast<IItemPort*>(target);
        if (!port) continue;

        auto type = port->get_port(source_tile);
        if (type != PortType::In && type != PortType::Both) continue;

        for (auto it = inventory.slots.begin(); it != inventory.slots.end(); ++it) {
            if (it->second <= 0) continue;  // maybe erase this too

            int item_sent = port->push_item({it->first, 1});
            if (item_sent == 0) continue;

            it->second -= item_sent;
            push_timer = PUSH_COOLDOWN;
            return;
        }
    }
    

    return;
}

void Chest::draw(Sprites &batch) const {
    Rect src{0.0f, 0.0f, 32.0f, 16.0f};
    //round to whole pixels so the sprite never sits between pixels:
    glm::vec2 top_left = glm::round(position);
    batch.draw(*box_texture, src, top_left);

}

auto Chest::hitbox() const -> Rect {
    return Rect{position.x, position.y, 32.0f, 16.0f};
}

void Chest::get_occupied(std::vector<glm::ivec2> *ans) const {
    auto tile = Tilemap::tile_at_point(position + glm::vec2{GameConfig::TILE_SIZE / 2.0f});
	ans->emplace_back(tile);
    ans->emplace_back(tile + glm::ivec2{1, 0});
}

void Chest::on_interact(Context &ctx) {
    // should trigger inventory display through some mechanism
    ctx.open_container = &inventory;
}

auto Chest::get_inventory() -> Inventory* {
    return &inventory;
}

IItemPort::PortType Chest::get_port(glm::ivec2 grid_coord) {
    auto tile = Tilemap::tile_at_point(position + glm::vec2{GameConfig::TILE_SIZE / 2.0f});
    if (grid_coord == tile + glm::ivec2{-1, 0}) {
        return IItemPort::PortType::In;
    }
    if (grid_coord == tile + glm::ivec2{2, 0}) {
        return IItemPort::PortType::Out;
    }
    return IItemPort::PortType::None;
}

int Chest::push_item(ItemRecord item_record) {
    return item_record.second - inventory.add(item_record.first, item_record.second);
}
