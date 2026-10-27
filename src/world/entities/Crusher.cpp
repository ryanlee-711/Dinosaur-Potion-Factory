#include "Crusher.hpp"
#include "../../starter/Load.hpp"
#include "../../starter/data_path.hpp"

#include <cmath>

static Load< Texture > box_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/sprites/machines/crusher.png"));
});


Crusher::Crusher() {
    inventory.emplace_back(ItemRecord{"nil", 0});
    inventory.emplace_back(ItemRecord{"nil", 0});
}

void Crusher::update(FrameInput const &input, float elapsed, Tilemap const &map, std::vector<std::unique_ptr<Entity>> const &entities) {

    // Processing
    processing_timer -= elapsed;
    if (processing_timer <= 0.0f) {
        if (inventory[0].first != "nil" && inventory[0].second > 0) {
            inventory[0].second--;

            if (inventory[1].first == "nil") {
                inventory[1].first = "apple_powder";  // hardcoding the only recipe now
                inventory[1].second = 0;
            }
            inventory[1].second++;
            if (inventory[0].second == 0) {
                inventory[0].first = "nil";
            }
            processing_timer = CRUSH_COOLDOWN;
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

    if (inventory[1].second <= 0) return;
    int item_sent = port->push_item({inventory[1].first, 1});

    if (item_sent != 0) {
        inventory[1].second -= item_sent;
        if (inventory[1].second <= 0) {
            inventory[1].first = "nil";
            inventory[1].second = 0;
        }
        push_timer = PUSH_COOLDOWN;
    }

    return;
}

void Crusher::draw(Sprites &batch) const {
    Rect src{0.0f, 0.0f, 32.0f, 48.0f};
    //round to whole pixels so the sprite never sits between pixels:
    glm::vec2 top_left = glm::round(position);
    batch.draw(*box_texture, src, top_left - glm::vec2(0.0f, 32.0f));

}

auto Crusher::hitbox() const -> Rect {
    return Rect{position.x, position.y, 32.0f, 16.0f};
}

void Crusher::get_occupied(std::vector<glm::ivec2> *ans) const {
    auto tile = Tilemap::tile_at_point(position + glm::vec2{GameConfig::TILE_SIZE / 2.0f});
	ans->emplace_back(tile);
    ans->emplace_back(tile + glm::ivec2{1, 0});
}

void Crusher::on_interact(Context &ctx) {
    // should trigger inventory display through some mechanism
    ctx.flag = Context::InteractionType::OpenInventory;
}

auto Crusher::get_inventory() -> Inventory* {
    return &inventory;
}

IItemPort::PortType Crusher::get_port(glm::ivec2 grid_coord) {
    auto tile = Tilemap::tile_at_point(position + glm::vec2{GameConfig::TILE_SIZE / 2.0f});
    if (grid_coord == tile + glm::ivec2{-1, 0}) {
        return IItemPort::PortType::In;
    }
    if (grid_coord == tile + glm::ivec2{2, 0}) {
        return IItemPort::PortType::Out;
    }
    return IItemPort::PortType::None;
}

int Crusher::push_item(ItemRecord item_record) {

    if (inventory[0].first == "nil") {
        inventory[0].first = item_record.first;
        processing_timer = CRUSH_COOLDOWN;
    }

    if (inventory[0].first == item_record.first) {
        inventory[0].second += 1;
        return 1;
    }
    return 0;
}