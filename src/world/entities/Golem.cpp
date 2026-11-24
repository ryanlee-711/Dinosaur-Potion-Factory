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
#include "Forageable.hpp"
#include "OreVein.hpp"
#include "Tree.hpp"

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
	moving = false;
	work_cooldown -= elapsed;
	absorb_item_cooldown -= elapsed;
	state_change_timeout -= elapsed;

	auto find_live = [&world](uint32_t uid) -> Entity * {
		auto it = world.uid_to_entities.find(uid);
		return it == world.uid_to_entities.end() || it->second->removed ? nullptr : it->second;
	};
	if (!initialized) reload_targets(world);
	auto *box = dynamic_cast<Machine *>(find_live(fuel_box));
	if (!box || box->get_machine_type() != "fuelbox") {
		removed = true;
		return;
	}
	Inventory &fuel = *box->get_inventory();

	if (absorb_item_cooldown <= 0.0f) {
		absorb_item_cooldown = 0.5f;
		for (auto &entity : world.entities) {
			auto *item = dynamic_cast<ItemEntity *>(entity.get());
			if (!item || item->removed) continue;
			if (glm::distance(item->position, position) > 3.5f * TILE_SIZE || glm::distance(item->position, box->position) > 8.0f * TILE_SIZE) continue;
			auto *source = item->get_inventory();
			auto id = source->get_slot_id(0);
			int count = source->get_slot_count(0);
			if (!id.empty() && count > 0) {
				int leftover = inventory.add(id, count);
				source->remove(id, count - leftover);
			}
			if (source->empty()) item->removed = true;
		}
	}

	if (!find_live(target_entity) || state_change_timeout <= 0.0f || (!inventory.empty() && target_entity != binded_chest) || (inventory.empty() && target_entity == binded_chest)) {
		reload_targets(world);
		state_change_timeout = 15.0f;
	}
	Entity *target = find_live(target_entity);
	if (!target) return;

	glm::vec2 direction = target->position - position;
	float distance = glm::length(direction);
	if (distance > 1.0f) {
		animation_timer += elapsed;
		facing = direction;
		position += direction / distance * std::min(WALK_SPEED * elapsed, distance);
		moving = true;
		return;
	}
	animation_timer = 0.0f;
	if (work_cooldown > 0.0f) return;
	work_cooldown = 0.1f;

	if (target_entity == binded_chest) {
		if (auto *chest = dynamic_cast<Machine *>(target)) {
			int sent = chest->push_item({inventory.get_slot_id(0), 1});
			inventory.extract_from_slot(0, sent);
			if (sent == 0 || inventory.empty()) reload_targets(world);
		}
		return;
	}
	if (!inventory.empty()) return;

	if (auto *plant = dynamic_cast<Forageable *>(target)) {
		if (!plant->show_hint() && fuel.has("haste_crystal") && plant->restore()) {
			fuel.remove("haste_crystal", 1);
		}
		// Forageable only uses the destination inventory in this context.
		// Copy the player to satisfy the existing API without touching player state.
		Player worker = world.player;
		worker.position = position;
		IInteractable::Context ctx{worker, inventory};
		plant->on_interact(ctx);
		reload_targets(world);
	} else if (auto *mineable = dynamic_cast<IMineable *>(target)) {
		if (!fuel.has("strength_crystal")) {
			reload_targets(world);
			return;
		}
		if (auto *vein = dynamic_cast<OreVein *>(target)) {
			if (!vein->show_hint() && fuel.has("haste_crystal") && vein->restore()) {
				fuel.remove("haste_crystal", 1);
			}
			if (!vein->show_hint()) {
				reload_targets(world);
				return;
			}
		} else if (auto *tree = dynamic_cast<Tree *>(target)) {
			if (!tree->show_hint() && fuel.has("haste_crystal") && tree->restore()) {
				fuel.remove("haste_crystal", 1);
			}
			// restore() refuses falling trees. Do not charge strength for idle hits.
			if (!tree->show_hint()) {
				reload_targets(world);
				return;
			}
		}

		if (!world.spawned) return;
		std::vector<ItemRecord> drops;
		if (mineable->mine(1, position, drops)) {
			if (std::bernoulli_distribution(0.05)(world.rng)) fuel.remove("strength_crystal", 1);
			for (auto const &[id, count] : drops) {
				int leftover = inventory.add(id, count);
				if (leftover > 0) {
					auto item = std::make_unique<ItemEntity>(id, leftover);
					item->position = position;
					world.spawned->push_back(std::move(item));
				}
			}
		}
		if (!inventory.empty()) reload_targets(world);
	}
}

void Golem::reload_targets(World const &world) {
	if (!initialized) {
		initialized = true;
		for (auto const &entity : world.entities) {
			if (entity->removed) continue;
			auto *machine = dynamic_cast<Machine *>(entity.get());
			if (machine && machine->get_machine_type() == "fuelbox" &&
			    glm::distance(machine->position, position) <= 8.0f * TILE_SIZE) {
				fuel_box = machine->uid;
				break;
			}
		}
	}

	target_entity = binded_chest = binded_producer = 0;


	auto found = world.uid_to_entities.find(fuel_box);
	if (found == world.uid_to_entities.end() || found->second->removed) return;
	auto *box = dynamic_cast<Machine *>(found->second);
	if (!box || box->get_machine_type() != "fuelbox") return;
	Inventory const &fuel = *box->get_inventory();
	bool has_strength = fuel.has("strength_crystal");
	bool has_haste = fuel.has("haste_crystal");
	float chest_distance = 3000.0f, producer_distance = 3000.0f;

	for (auto const &entity : world.entities) {
		Entity *ptr = entity.get();
		if (ptr->removed || glm::distance(ptr->position, box->position) > 8.0f * TILE_SIZE) continue;
		float distance = glm::distance(ptr->position, position);
		if (auto *machine = dynamic_cast<Machine *>(ptr)) {
			// the target chest should have space for stuff
			if (machine->get_machine_type() == "chest" && distance < chest_distance && 
			    (inventory.empty() || machine->get_inventory()->space_for(inventory.get_slot_id(0)) > 0)) {
				binded_chest = ptr->uid;
				chest_distance = distance;
			}
		}

		bool workable = false;
		// Forageable-->can grow
		// Mineable --> must use fuel
		if (auto *plant = dynamic_cast<Forageable *>(ptr)) {
			workable = plant->show_hint() || has_haste;
		} else if (dynamic_cast<IMineable *>(ptr) != nullptr) {
			auto *hint = dynamic_cast<IHintable *>(ptr);
			workable = has_strength && (!hint || hint->show_hint() || has_haste);
		} else if (auto *item = dynamic_cast<ItemEntity *>(ptr)) {
			workable = !item->get_inventory()->empty() &&
			    inventory.space_for(item->get_inventory()->get_slot_id(0)) > 0;
		} else {
			workable = dynamic_cast<IWorkable *>(ptr) != nullptr;
		}
		if (workable && distance < producer_distance) {
			binded_producer = ptr->uid;
			producer_distance = distance;
		}
	}
	target_entity = inventory.empty() ? binded_producer : binded_chest;
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
	ss << ' ' << fuel_box;
	item_id_t carried = inventory.get_slot_id(0);
	ss << ' ' << (carried.empty() ? "null" : carried) << ' ' << inventory.get_slot_count(0);
    return ss.str();
}

void Golem::load(std::string line) {
    std::istringstream ss(line);

    /**
     * Store Format:
     *   <uid> golem <pos.x> <pos.y> <fuelbox_uid> <carried_item> <carried_count>
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
	uint32_t saved_box;
	item_id_t carried;
	int count;
	if (!(ss >> saved_box >> carried >> count)) {
		throw std::runtime_error{"Didn't read golem fuelbox / inventory"};
	}
	bind_fuelbox(saved_box);
	if (carried == "null" || count <= 0) inventory.set_slot(0, "", 0);
	else inventory.set_slot(0, carried, count);
	target_entity = binded_chest = binded_producer = 0;
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
