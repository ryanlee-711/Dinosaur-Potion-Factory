#include "Pipe.hpp"
#include "../World.hpp"
#include "../TileMap.hpp"
#include "../../starter/Load.hpp"
#include "../../starter/data_path.hpp"

#include "../Components.hpp"

#include <cmath>

static Load< Texture > box_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/sprites/machines/pipe.png"));
});

Pipe::Pipe() : inventory(1), frame(0), base_frame(0) {
    in_dir = glm::ivec2{-1, 0};
    out_dir = glm::ivec2(1, 0);
}

void Pipe::update(FrameInput const &input, float elapsed, World const &world) {
    auto const &entities = world.entities;
    push_timer -= elapsed;
    if (push_timer <= 0.0f) {
        push_timer = 0.0f;
    } else {
        return;
    }

    // push item to the right
    auto origin = Tilemap::tile_at_point(position + glm::vec2{GameConfig::TILE_SIZE / 2.0f});

    auto source_tile = origin + glm::ivec2(0, 0);
    auto target_tile = origin + out_dir;

    Entity *target = entity_at(entities, target_tile);
    if (!target || target == this) return;

    // get IItemPort!
    auto *port = dynamic_cast<IItemPort*>(target);
    if (!port) return;

    auto type = port->get_port(source_tile);
    if (type != PortType::In && type != PortType::Both) return;

    for (auto it = inventory.slots.begin(); it != inventory.slots.end(); ++it) {
        if (it->second <= 0) continue;  // maybe erase this too

        int item_sent = port->push_item({it->first, 1});
        if (item_sent == 0) continue;

        it->second -= item_sent;
        if (it->second == 0) {
            frame = base_frame;  // reset sprite
        }
        return;
    }

    return;
}

void Pipe::draw(Sprites &batch) const {
    Rect src{frame * 16.0f, 0.0f, 16.0f, 16.0f};
    //round to whole pixels so the sprite never sits between pixels:
    glm::vec2 top_left = glm::round(position);
    batch.draw(*box_texture, src, top_left);

}

auto Pipe::hitbox() const -> Rect {
    return Rect{position.x, position.y, 16.0f, 16.0f};
}

void Pipe::get_occupied(std::vector<glm::ivec2> *ans) const {
    auto tile = Tilemap::tile_at_point(position + glm::vec2{GameConfig::TILE_SIZE / 2.0f});
	ans->emplace_back(tile);
}

void Pipe::on_interact(Context &ctx) {
    // should add the item to the player's inventory, if there is any item
    // TODO
    configure(base_frame > 0 ? 0 : 1);
}

auto Pipe::get_inventory() -> Inventory* {
    return &inventory;
}

IItemPort::PortType Pipe::get_port(glm::ivec2 grid_coord) {
    auto tile = Tilemap::tile_at_point(position + glm::vec2{GameConfig::TILE_SIZE / 2.0f});
    if (grid_coord == tile + in_dir) {
        return IItemPort::PortType::In;
    }
    if (grid_coord == tile + out_dir) {
        return IItemPort::PortType::Out;
    }
    return IItemPort::PortType::None;
}

int Pipe::push_item(ItemRecord item_record) {

    int val = inventory.add(item_record.first, item_record.second);
    if (item_record.second - val > 0)
        frame = base_frame + 6;
        push_timer = PUSH_COOLDOWN;

    return item_record.second - val;
}