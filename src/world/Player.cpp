#include "Player.hpp"

#include "TileMap.hpp"
#include "World.hpp"
#include "../main/GameConfig.hpp"
#include "../starter/Load.hpp"
#include "../starter/data_path.hpp"
#include "../inventory/Item.hpp"
#include "../rendering/Particles.hpp"

#include <algorithm>
#include <cmath>
#include <vector>
#include <sstream>

using GameConfig::TILE_SIZE;

//Player art: one row of frames, each FRAME_W x FRAME_H, in the order down, up, left, right.
// (Placeholder until the sprite/animation system replaces it.)
static Load< Texture > player_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/sprites/dwarf.png"));
});
static constexpr int PLAYER_FRAME_W = 32;
static constexpr int PLAYER_FRAME_H = 32;

void Player::update(FrameInput const &input, float elapsed, World const &world) {
	// count down effect timers every frame, and drop the ones that ran out
	for (auto it = effects.begin(); it != effects.end(); ) {
		it->second.time_left -= elapsed;
		if (it->second.time_left <= 0.0f) it = effects.erase(it);
		else ++it;
	}

	collision_mask = 0b1111;
	//update collision layer based on effects
	for (auto it = effects.begin(); it != effects.end(); ++it) {
		if (it->first == "float") {
			collision_mask = 0b1;
		}
	}

	auto direction = input.direction;
	//diagonal input shouldn't be faster than straight input
	if (glm::length(direction) > 1.0f) direction = glm::normalize(direction);

	moving = (direction != glm::vec2(0.0f));
	if (!moving) return;

	// Only update animation timer when moving
	animation_timer += elapsed * (input.sprint ? 1.5f : 1.0f);
	if (animation_timer > 60.0f) animation_timer -= 60.0f;

	//face whichever way we're mostly going:
	if (std::abs(direction.x) > std::abs(direction.y)) {
		facing = glm::ivec2(direction.x > 0.0f ? 1 : -1, 0);
	} else {
		facing = glm::ivec2(0, direction.y > 0.0f ? 1 : -1);
	}

	float speed = WALK_SPEED;
	speed *= 1.0f + SPEED_PER_LEVEL * float(effect_level("speed")); // "speed" effect
	if (input.sprint) speed *= SPRINT_MULTIPLIER;  // Shift; stacks with the boost
	// in the peak, move speed is decreased
	if (position.y < 750.0f && direction.y <= 0.0f && effect_level("warmth") == 0) {
		speed *= std::clamp((position.y - 600.0f) / 150.0f,  0.0f, 1.0f);
	}
	//move one axis at a time, this is what lets you slide along walls instead of sticking to them when moving diagonally
	glm::vec2 step = direction * speed * elapsed;
	glm::vec2 before = position;
	move_axis(0, step.x, world);
	move_axis(1, step.y, world);

	dust_distance += glm::length(position - before);
	if (dust_distance >= DUST_STEP) {
		dust_distance -= DUST_STEP;
		world.particles->burst(Presets::StepDust, position - glm::vec2(facing) * 4.0f);
	}
}

void Player::move_axis(int axis, float amount, World const &world) {
	if (amount == 0.0f) return;

	//if we're somehow already stuck inside a wall, let the player walk out of it
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

void Player::draw(Sprites &batch) const {
    int frame = 3; //down
	if (facing.y < 0) frame = 1;      //up
	else if (facing.x < 0) frame = 2; //left
	else if (facing.x > 0) frame = 0; //right

	int anim_frame = static_cast<int>(animation_timer * 8.0f) % 8;

	Rect src{float(anim_frame * PLAYER_FRAME_W), float(frame * PLAYER_FRAME_H), float(PLAYER_FRAME_W), float(PLAYER_FRAME_H)};
	//round to whole pixels so the sprite never sits between pixels:
	glm::vec2 top_left = glm::round(position) - glm::vec2(PLAYER_FRAME_W * 0.5f, float(PLAYER_FRAME_H));
	
	if (facing.y >= 0) {
		batch.draw(*player_texture, src, top_left);
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
		batch.draw(*player_texture, src, top_left);
	}
	
}

Rect Player::hitbox() const {
	return Rect{position.x - HITBOX_W * 0.5f, position.y - HITBOX_H, HITBOX_W, HITBOX_H};
}

Rect Player::interact_box() const {
	Rect hb = hitbox();
	if (facing.x > 0) return Rect{hb.x + hb.w, hb.y, REACH, hb.h};  //right
	if (facing.x < 0) return Rect{hb.x - REACH, hb.y, REACH, hb.h}; //left
	if (facing.y < 0) return Rect{hb.x, hb.y - REACH, hb.w, REACH}; //up
	return Rect{hb.x, hb.y + hb.h, hb.w, REACH};                    //down
}


std::string Player::save() const {
    std::ostringstream ss;
    ss << uid;
	ss << ' ' << "player";
    ss << ' ' << position.x << ' ' << position.y;
	ss << ' ' << effects.size();
	for (auto &[effect_id, effect] : effects) {
		ss << ' ' << effect_id << ' ' << effect.level << ' ' << effect.time_left;
	}
    return ss.str();
}

void Player::load(std::string line) {
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
	int effect_count;
	if (!(ss >> effect_count)) {
        throw std::runtime_error{"Invalid effect count"};
    }

	effects.clear();
	for (int idx = 0; idx < effect_count; ++idx) {
		effect_id_t effect;
		int level;
		float length;
		if (!(ss >> effect >> level >> length)) {
			throw std::runtime_error{"Invalid effect"};
		}
		add_effect(effect, length, level);
	}


	held_item.first = "";
	held_item.second = 0;
}

glm::ivec2 Player::facing_tile() const {
	//measure from the middle of the hitbox so "in front" matches what you see
	glm::vec2 middle = position - glm::vec2(0.0f, HITBOX_H * 0.5f);
	return Tilemap::tile_at_point(middle) + facing;
}

void Player::get_occupied(std::vector<glm::ivec2> *ans) const {
	glm::vec2 middle = position - glm::vec2(0.0f, HITBOX_H * 0.5f);

	ans->emplace_back(Tilemap::tile_at_point(middle));

}

void Player::add_effect(effect_id_t const &id, float seconds, int level) {
	if (seconds <= 0.0f || level <= 0) return;
	ActiveEffect &active = effects[id]; // creates {0s, level 0} if the player doesn't have it yet
	if (level > active.level) {
		active.level = level;
		active.time_left = seconds;
	} else if (level == active.level) {
		active.time_left = std::max(active.time_left, seconds); // same level: keep the longer timer
	}
}

int Player::effect_level(effect_id_t const &id) const {
	auto it = effects.find(id);
	return it != effects.end() ? it->second.level : 0;
}