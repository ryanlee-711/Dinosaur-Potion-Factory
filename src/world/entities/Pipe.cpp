#include "Pipe.hpp"
#include "../World.hpp"
#include "../TileMap.hpp"
#include "../../starter/Load.hpp"
#include "../../starter/data_path.hpp"

#include "../Components.hpp"


#include <array>
#include <cmath>
#include <sstream>

static Load< Texture > box_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/sprites/machines/pipe.png"));
});

Pipe::Pipe() : inventory(1), frame(0), base_frame(0) {
    in_dir = glm::ivec2{0, 0};
    out_dir = glm::ivec2(0, 0);
}

void Pipe::update(FrameInput const &input, float elapsed, World const &world) {
    auto origin = Tilemap::tile_at_point(position + glm::vec2{GameConfig::TILE_SIZE / 2.0f});
    if (queue_reconnect) {
        queue_reconnect = false;
        // do reconnection.
        auto const dirs = std::array<glm::ivec2, 4>{glm::ivec2{-1, 0}, glm::ivec2{0, -1}, glm::ivec2{1, 0}, glm::ivec2{0, 1}};

        // for each direction, look up neighbor there and check its mode
        auto old_in = in_dir;
        auto old_out = out_dir;
        in_dir = glm::ivec2{0};
        out_dir = glm::ivec2{0};
        for (auto dir : dirs) {
            glm::ivec2 probe_tile = origin + dir;
            if (auto entity = world.entity_at(probe_tile); entity) {
                if (auto *port = dynamic_cast<IItemPort*>(entity); port) {
                    // this is a port, so lookup own coord
                    PortType ans = port->get_port(origin);
                    if (ans == PortType::In) {
                        out_dir = dir;
                        continue;
                    } else if (ans == PortType::Out) {
                        in_dir = dir;
                        continue;
                    }
                }
                if (auto *other = dynamic_cast<Pipe*>(entity); other) {
                    if (other->in_dir == glm::ivec2{0} && other->out_dir != glm::ivec2{0}) {
                        out_dir = dir;
                        other->configure(0);
                    } else if (other->out_dir == glm::ivec2{0} && other->in_dir != glm::ivec2{0}) {
                        in_dir = dir;
                        other->configure(0);
                    }
                }
            }
        }

        // propagate update to neighbors...
        if (in_dir != old_in || out_dir != old_out) {
            for (auto dir : dirs) {
                if (auto *other = dynamic_cast<Pipe*>(world.entity_at(origin + dir)); other) {
                    other->configure(0);
                }
            }
        }

        // update frame.

        auto direction_bit = [](glm::ivec2 d) {
            if (d.x < 0) return 1; // W
            if (d.y < 0) return 2; // N
            if (d.x > 0) return 4; // E
            if (d.y > 0) return 8; // S
            return 0;
        };
        // ...............................................NW....WE.NE...
        static constexpr int which_frame[16] = {0, 0, 1, 5, 0, 0, 4, -1, 1, 3, 1, -1, 2, -1, -1, -1};

        size_t mask = 0x0u;
        mask |= direction_bit(in_dir);
        mask |= direction_bit(out_dir);
        base_frame = which_frame[mask];
        frame = base_frame;

    }


    push_timer -= elapsed;
    if (push_timer <= 0.0f) {
        push_timer = 0.0f;
    } else {
        return;
    }

    // push item to the right

    auto source_tile = origin + glm::ivec2(0, 0);
    auto target_tile = origin + out_dir;

    Entity *target = world.entity_at(target_tile);
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

void Pipe::draw_hint(Sprites &batch) const {
    // Redraw it, for some reason.
    Rect src{frame * 16.0f, 0.0f, 16.0f, 16.0f};
    glm::vec2 top_left = glm::round(position);
    batch.draw(*box_texture, src, top_left, glm::u8vec4{0xff, 0xff, 0x00, 0x80});
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
    configure(0);
}

auto Pipe::get_inventory() -> Inventory* {
    return &inventory;
}

IItemPort::PortType Pipe::get_port(glm::ivec2 grid_coord) {
    auto tile = Tilemap::tile_at_point(position + glm::vec2{GameConfig::TILE_SIZE / 2.0f});
    if (grid_coord == tile + in_dir && in_dir != glm::ivec2{0}) {
        return IItemPort::PortType::In;
    }
    if (grid_coord == tile + out_dir && out_dir != glm::ivec2{0}) {
        return IItemPort::PortType::Out;
    }
    return IItemPort::PortType::None;
}

int Pipe::push_item(ItemRecord item_record) {

    int val = inventory.add(item_record.first, item_record.second);
    if (item_record.second - val > 0) {
        frame = base_frame + 6;
        push_timer = PUSH_COOLDOWN;
    }
    return item_record.second - val;
}


std::string Pipe::save() const {
    std::ostringstream ss;
    ss << uid;
    ss << ' ' << "pipe";
    ss << ' ' << position.x << ' ' << position.y << ' ' << in_dir.x << ' ' << in_dir.y << ' ' << out_dir.x << ' ' << out_dir.y;
    for (int idx = 0; idx < int(inventory.size()); ++idx) {
        item_id_t item_id = inventory.get_slot_id(idx);
        int count = inventory.get_slot_count(idx);
        if (item_id == "") item_id = "null";
        ss << ' ' << item_id << ' ' << count;
    }
    return ss.str();
}

void Pipe::load(std::string line) {
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

    if (!(ss >> position.x >> position.y >> in_dir.x >> in_dir.y >> out_dir.x >> out_dir.y)) {
        throw std::runtime_error{"Didn't read position / in_dir / out_dir"};
    }

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