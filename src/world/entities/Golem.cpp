#include "Golem.hpp"

#include "../TileMap.hpp"
#include "../World.hpp"
#include "../../main/GameConfig.hpp"
#include "../../starter/Load.hpp"
#include "../../starter/data_path.hpp"
#include "../../inventory/Item.hpp"
#include "../ItemEntity.hpp"
#include "../../rendering/Particles.hpp"
#include "Machine.hpp"

#include <algorithm>
#include <cmath>
#include <vector>
#include <sstream>

using GameConfig::TILE_SIZE;

//Golem art: one row of frames, each FRAME_W x FRAME_H, in the order down, up, left, right.
// (Placeholder until the sprite/animation system replaces it.)
static Load< Texture > golem_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/sprites/entities/golem.png"));
});
static constexpr int PLAYER_FRAME_W = 16;
static constexpr int PLAYER_FRAME_H = 16;

Golem::Golem(): inventory(1) { }

void Golem::update(FrameInput const &input, float elapsed, World const &world) {
	work_cooldown -= elapsed;
	absorb_item_cooldown -= elapsed;

	// Check target entity. If expired, it must rediscover a new one.
	if (!world.uid_to_entities.contains(target_entity) || state_change_timeout < 0.0f) {
		// Discover new target entity
		reload_targets(world);
		state_change_timeout = 15.0f;
	}

	// If the fuel box is no longer there, don't act at all
	if (!world.uid_to_entities.contains(fuel_box)) {
		removed = true;
		return;
	}

	// If there is still no target, return immediately
	if (target_entity == 0) return;

	// Clear items within a radius
	if (absorb_item_cooldown < 0.0f) {
		absorb_item_cooldown = 0.5f;
		for (auto &entity : world.entities) {
			if (auto* item = dynamic_cast<ItemEntity*>(entity.get())) {
				if (item->removed) {
					target_entity = 0;
					return;
				}

				if (glm::length(item->position - position) > 2.5f * TILE_SIZE) {
					continue;
				}

				auto* source = item->get_inventory();
				auto id = source->get_slot_id(0);  // copy before modifying source
				int count = source->get_slot_count(0);

				if (!id.empty() && count > 0) {
					int leftover = inventory.add(id, count);
					int collected = count - leftover;
					source->remove(id, collected);
				}

				if (source->empty()) item->removed = true;
			}
		}
	}


	// Move towards the target entity
	glm::vec2 direction = world.uid_to_entities.at(target_entity)->position - position;
	if (glm::length(direction) > 1.0f) {
		// move
		animation_timer += elapsed;

		// update facing
		facing = direction;

		glm::vec2 step = glm::normalize(direction) * WALK_SPEED * elapsed;
		position += step;  // no collision. I am NOT writing A-star here!
		return;
	}

	animation_timer = 0.0f;

	// If within a distance from the target, starts working.
	state_change_timeout -= elapsed;
	if (work_cooldown < 0.0f) {
		work_cooldown = 0.1f;
		// WORK
		if (target_entity == binded_chest) {
			auto chest = dynamic_cast<Machine *>(world.uid_to_entities.at(target_entity));
			if (chest) {
				// put thing inside
				if (inventory.slots[0].second <= 0) {
					reload_targets(world);
					return;
				}
				int item_sent = chest->push_item({inventory.slots[0].first, 1});

				if (item_sent != 0) {
					inventory.slots[0].second -= item_sent;
					if (inventory.slots[0].second <= 0) {
						inventory.slots[0].first = "";
						inventory.slots[0].second = 0;
					}
				}
			}
			return;
		}
		// Target is a producer. Can try to use stored item on it...

	}
}

void Golem::reload_targets(World const &world) {
	// First, recheck that both the producer and the chest is not expired
	if (!initialized) {
		// Only initialize once in its entire lifetime. Failure means it is frozen forever
		initialized = true;
		for (auto const &entity : world.entities) {
			auto ptr = entity.get();
			if (auto machine = dynamic_cast<Machine *>(ptr)) {
				// Check if it is chest, else continue
				if (machine->get_machine_type() != "fuelbox") continue;
				float distance;
				if (distance = glm::distance(machine->position, position); distance > 8.0f * TILE_SIZE) continue;

				// add it as fuelbox
				fuel_box = machine->uid;
				break;
			}
		}
	}

	if (world.uid_to_entities.count(fuel_box) == 0) return;

	auto fuel_box_ptr = world.uid_to_entities.at(fuel_box);

	if (!world.uid_to_entities.contains(binded_chest) || !world.uid_to_entities.contains(binded_producer)) {
		// rebind. take a bit longer to enumerate all

		Entity *nearest_chest = nullptr;
		float nearest_chest_distance = 3000.0f;
		Entity *nearest_producer = nullptr;
		float nearest_producer_distance = 3000.0f;

		for (auto const &entity : world.entities) {
			auto ptr = entity.get();
			if (auto machine = dynamic_cast<Machine *>(ptr)) {
				// Check if it is chest, else continue
				if (machine->get_machine_type() != "chest") continue;
				float distance;
				if (distance = glm::distance(machine->position, fuel_box_ptr->position); distance > 8.0f * TILE_SIZE) continue;
				if (distance > nearest_chest_distance) continue;
				nearest_chest_distance = distance;
				nearest_chest = machine;
			}

			if (dynamic_cast<IWorkable *>(ptr) != nullptr) {
				float distance;
				if (distance = glm::distance(ptr->position, fuel_box_ptr->position); distance > 8.0f * TILE_SIZE) continue;
				if (distance > nearest_producer_distance) continue;
				nearest_producer = ptr;
				nearest_producer_distance = distance;
			}
		}
		// Overwrite existing values.
		if (nearest_chest) binded_chest = nearest_chest->uid;
		else binded_chest = 0;
		if (nearest_producer) binded_producer = nearest_producer->uid;
		else binded_producer = 0;
	}

	// If neither are valid, just give up
	if (binded_chest == 0 && binded_producer == 0) {
		target_entity = 0;
		return;
	}

	// Check which ones are actually valid. If one is invalid, use the other.
	if (binded_chest == 0 && binded_producer != 0) {
		target_entity = binded_producer;
		return;
	} else if (binded_chest != 0 && binded_producer == 0) {
		target_entity = binded_chest;
		return;
	}

	// If both are valid, binding is based on inventory fullness, fuel usage or timeout.
	if (inventory.get_slot_count(0) <= 0) {
		target_entity = binded_producer;
		return;
	}

	if (state_change_timeout - static_cast<float>(inventory.get_slot_count(0)) < 0.0f) {
		target_entity = binded_chest;
		return;
	}
}



void Golem::move_axis(int axis, float amount, World const &world) {
	if (amount == 0.0f) return;

	//if we're somehow already stuck inside a wall, let the golem walk out of it
	constexpr float STUCK_EPS = 0.01f;
	Rect before = hitbox();
	Rect inner{before.x + STUCK_EPS, before.y + STUCK_EPS, before.w - 2.0f * STUCK_EPS, before.h - 2.0f * STUCK_EPS};
	bool was_stuck = world.blocked(inner, *this);

	position[axis] += amount;
	if (was_stuck) return;

	//anything we overlap now
	Rect box = hitbox();
	std::vector< Rect > hits;
	world.solids_overlapping(box, *this, hits);
	if (hits.empty()) return;

	//push back so we exactly touch the nearest thing we hit.
	constexpr float SKIN = 0.001f;
	if (amount > 0.0f) {
		float edge = hits[0].min()[axis];
		for (Rect const &r : hits) edge = std::min(edge, r.min()[axis]);
		position[axis] -= (box.max()[axis] - edge) + SKIN;
	} else {
		float edge = hits[0].max()[axis];
		for (Rect const &r : hits) edge = std::max(edge, r.max()[axis]);
		position[axis] += (edge - box.min()[axis]) + SKIN;
	}
}

void Golem::draw(Sprites &batch) const {
    int frame = 3; //down
	if (facing.y < 0) frame = 1;      //up
	else if (facing.x < 0) frame = 2; //left
	else if (facing.x > 0) frame = 0; //right

	int anim_frame = static_cast<int>(animation_timer * 8.0f) % 4;

	Rect src{float(anim_frame * PLAYER_FRAME_W), float(frame * PLAYER_FRAME_H), float(PLAYER_FRAME_W), float(PLAYER_FRAME_H)};
	//round to whole pixels so the sprite never sits between pixels:
	glm::vec2 top_left = glm::round(position) - glm::vec2(PLAYER_FRAME_W * 0.5f, float(PLAYER_FRAME_H));

	if (facing.y >= 0) {
		batch.draw(*golem_texture, src, top_left);
	}
	// draw held item
	if (held_item.first != "" && held_item.second > 0) {
		if (auto item = Item::get(held_item.first); item != nullptr) {
			glm::vec2 offset = glm::vec2{-2.0f, 15.0f};
			if (facing.y < 0) offset = glm::vec2{19.0f, 13.0f};
			if (facing.x < 0) offset = glm::vec2{6.0f, 16.0f};
			if (facing.x > 0) offset = glm::vec2{10.0f, 16.0f};

			batch.draw(*item->texture, Rect{0.0f, 0.0f, 16.0f, 16.0f}, top_left + offset);
		}
	}
	if (facing.y < 0) {
		batch.draw(*golem_texture, src, top_left);
	}

}

Rect Golem::hitbox() const {
	return Rect{position.x - HITBOX_W * 0.5f, position.y - HITBOX_H, HITBOX_W, HITBOX_H};
}

Rect Golem::interact_box() const {
	Rect hb = hitbox();
	if (facing.x > 0) return Rect{hb.x + hb.w, hb.y, REACH, hb.h};  //right
	if (facing.x < 0) return Rect{hb.x - REACH, hb.y, REACH, hb.h}; //left
	if (facing.y < 0) return Rect{hb.x, hb.y - REACH, hb.w, REACH}; //up
	return Rect{hb.x, hb.y + hb.h, hb.w, REACH};                    //down
}


std::string Golem::save() const {
    std::ostringstream ss;
    ss << uid;
	ss << ' ' << "golem";
    ss << ' ' << position.x << ' ' << position.y;
    return ss.str();
}

void Golem::load(std::string line) {
    std::istringstream ss(line);

    /**
     * Store Format:
     *   <uid> <pos.x> <pos.y> <effect_count> [EFFECTS]
     */
    uint32_t read_uid;
	std::string entity_type;

    if (!(ss >> read_uid >> entity_type)) {
        throw std::runtime_error{"UID read failure"};
    }
    if (read_uid != uid && uid != 0) {
        throw std::runtime_error{"Loaded data mismatches the entity's UID!"};
    }

    if (!(ss >> position.x >> position.y)) {
        throw std::runtime_error{"Didn't read position"};
    }


	held_item.first = "";
	held_item.second = 0;
}

glm::ivec2 Golem::facing_tile() const {
	//measure from the middle of the hitbox so "in front" matches what you see
	glm::vec2 middle = position - glm::vec2(0.0f, HITBOX_H * 0.5f);
	return Tilemap::tile_at_point(middle) + facing;
}

void Golem::get_occupied(std::vector<glm::ivec2> *ans) const {
	glm::vec2 middle = position - glm::vec2(0.0f, HITBOX_H * 0.5f);

	ans->emplace_back(Tilemap::tile_at_point(middle));

}