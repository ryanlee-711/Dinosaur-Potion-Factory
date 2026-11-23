#include "Splitter.hpp"
#include "../World.hpp"
#include "../TileMap.hpp"
#include "../../starter/Load.hpp"
#include "../../starter/data_path.hpp"

#include "../Components.hpp"


#include <array>
#include <cmath>
#include <sstream>

static Load< Texture > box_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/sprites/machines/splitter.png"));
});

Splitter::Splitter() : inventory(2), frame(0), base_frame(0) {

}

void Splitter::update(FrameInput const &input, float elapsed, World const &world) {
    auto origin = Tilemap::tile_at_point(position + glm::vec2{GameConfig::TILE_SIZE / 2.0f});

    push_timer -= elapsed;
    if (push_timer <= 0.0f) {
        push_timer = 0.0f;
    } else {
        return;
    }

    // push item to somewhere (depending on which work mode it owns)

    constexpr std::array<glm::ivec2, 4> dirs { glm::ivec2{1, 0}, glm::ivec2{0, -1}, glm::ivec2{-1, 0}, glm::ivec2{0, 1} };
    auto source_tile = origin + glm::ivec2(0, 0);

    if (work_mode == WorkMode::JOIN) {
        auto target_tile = origin + dirs[static_cast<int>(rotation)];
        Entity *target = world.entity_at(target_tile);
        if (!target || target == this) return;

        // push to it directly. same as pipe.
        auto *port = dynamic_cast<IItemPort*>(target);
        if (!port) return;

        auto type = port->get_port(source_tile);
        if (type != PortType::In && type != PortType::Both) return;
        for (auto it = inventory.slots.begin(); it != inventory.slots.end(); ++it) {
            if (it->second <= 0) break;  // doesn't check the second slot at all
            int item_sent = port->push_item({it->first, 1});
            if (item_sent == 0) break;
            it->second -= item_sent;

            push_timer = PUSH_COOLDOWN;
            return;
        }
    } else if (work_mode == WorkMode::CROSSOVER) {
        // needs to poll all directions
        for (int delta = 0; delta < 2; ++delta) { // both dir and dir+90degrees
            auto target_tile = origin + dirs[(static_cast<int>(rotation) + 1) % 4];
            Entity *target = world.entity_at(target_tile);
            if (!target || target == this) continue;

            // push by index
            auto *port = dynamic_cast<IItemPort*>(target);
            if (!port) continue;

            // push! push! push!
            auto type = port->get_port(source_tile);
            if (type != PortType::In && type != PortType::Both) continue;
            int idx = (static_cast<int>(rotation) + delta) % 2;
            if (inventory.slots[idx].second <= 0) continue;
            int item_sent = port->push_item({inventory.slots[idx].first, 1});
            if (item_sent == 0) continue;
            inventory.slots[idx].second -= item_sent;
            push_timer = PUSH_COOLDOWN;
        }
    } else {
        // must poll all different directions
        for (int delta = 0; delta < 4; ++delta) {

            int polled_index = (static_cast<int>(output_poll) + delta) % 4;

            auto target_tile = origin + dirs[polled_index];

            Entity *target = world.entity_at(target_tile);
            if (!target || target == this) continue;

            auto *port = dynamic_cast<IItemPort*>(target);
            if (!port) continue;

            auto type = port->get_port(source_tile);
            if (type != PortType::In && type != PortType::Both) continue;
            if (inventory.slots[0].second <= 0) continue;
            int item_sent = port->push_item({inventory.slots[0].first, 1});
            if (item_sent == 0) continue;
            inventory.slots[0].second -= item_sent;
            push_timer = PUSH_COOLDOWN;

            // poll to next index
            output_poll = static_cast<Rotation>((polled_index + 1) % 4);
            break;
        }
    }


    return;
}

void Splitter::draw(Sprites &batch) const {
    Rect src{static_cast<int>(rotation) * 16.0f, static_cast<int>(work_mode) * 16.0f, 16.0f, 16.0f};
    //round to whole pixels so the sprite never sits between pixels:
    glm::vec2 top_left = glm::round(position);
    batch.draw(*box_texture, src, top_left);

}

void Splitter::draw_hint(Sprites &batch) const {
    // Redraw it, for some reason.
    Rect src{frame * 16.0f, 0.0f, 16.0f, 16.0f};
    glm::vec2 top_left = glm::round(position);
    batch.draw(*box_texture, src, top_left, glm::u8vec4{0xff, 0xff, 0x00, 0x80});
}

auto Splitter::hitbox() const -> Rect {
    return Rect{position.x, position.y, 16.0f, 16.0f};
}

void Splitter::get_occupied(std::vector<glm::ivec2> *ans) const {
    auto tile = Tilemap::tile_at_point(position + glm::vec2{GameConfig::TILE_SIZE / 2.0f});
	ans->emplace_back(tile);
}

void Splitter::on_interact(Context &ctx) {
    // do nothing I guess
}

auto Splitter::get_inventory() -> Inventory* {
    return &inventory;
}

IItemPort::PortType Splitter::get_port(glm::ivec2 grid_coord) {
    last_push_query = grid_coord;
    auto tile = Tilemap::tile_at_point(position + glm::vec2{GameConfig::TILE_SIZE / 2.0f});
    constexpr std::array<glm::ivec2, 4> dirs { glm::ivec2{1, 0}, glm::ivec2{0, -1}, glm::ivec2{-1, 0}, glm::ivec2{0, 1} };

    for (size_t idx = 0; idx < 4; ++idx) {
        if (grid_coord != tile + dirs[idx]) continue;
        if (work_mode == WorkMode::JOIN) {
            if (idx == static_cast<size_t>(rotation)) {
                return PortType::Out;
            } else {
                return PortType::In;
            }
        } else if (work_mode == WorkMode::SPLIT) {
            if ((idx + 2) % 4 == static_cast<size_t>(rotation)) {
                return PortType::In;
            } else {
                return PortType::Out;
            }
        } else {
            if (static_cast<size_t>(rotation) == idx || static_cast<size_t>(rotation) == (idx + 1) % 4) {
                return PortType::In;
            } else {
                return PortType::Out;
            }
        }
    }
    return IItemPort::PortType::None;
}

int Splitter::push_item(ItemRecord item_record) {

    auto tile = Tilemap::tile_at_point(position + glm::vec2{GameConfig::TILE_SIZE / 2.0f});
    size_t slot_index = 0;
    // When this is a crossover pipe, the index for saving depends on outgoing direction
    if (work_mode == WorkMode::CROSSOVER && (last_push_query.y - tile.y != 0)) {
        slot_index = 1;
    }
    int val = inventory.add_to_slot_range(item_record.first, item_record.second, slot_index, slot_index + 1);
    if (item_record.second - val > 0) {
        frame = base_frame + 6;
        push_timer = PUSH_COOLDOWN;
    }
    return item_record.second - val;
}

item_id_t Splitter::get_drop_id() {
    if (work_mode == WorkMode::CROSSOVER) {
        return "crossover";
    } else if (work_mode == WorkMode::JOIN) {
        return "joiner";
    } else {
        return "splitter";
    }
}

std::string Splitter::save() const {
    std::ostringstream ss;
    ss << uid;
    ss << ' ' << "splitter";
    ss << ' ' << static_cast<int>(rotation);
    for (int idx = 0; idx < int(inventory.size()); ++idx) {
        item_id_t item_id = inventory.get_slot_id(idx);
        int count = inventory.get_slot_count(idx);
        if (item_id == "") item_id = "null";
        ss << ' ' << item_id << ' ' << count;
    }
    return ss.str();
}

void Splitter::load(std::string line) {
    std::istringstream ss(line);

    /**
     * Store Format:
     *   <uid> <pos.x> <pos.y> [IN] [OUT] [Inventory]
     */
    uint32_t read_uid;
    std::string entity_type;

    if (!(ss >> read_uid >> entity_type)) {
        throw std::runtime_error{"UID read failure"};
    }
    if (read_uid != uid && uid != 0) {
        throw std::runtime_error{"Loaded data mismatches the entity's UID!"};
    }

    int rot;
    if (!(ss >> position.x >> position.y >> rot)) {
        throw std::runtime_error{"Didn't read position / rot"};
    }
    rotation = static_cast<Rotation>(rot);

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