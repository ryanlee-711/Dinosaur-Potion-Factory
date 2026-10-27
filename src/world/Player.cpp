#include "Player.hpp"

#include "TileMap.hpp"
#include "World.hpp"
#include "../main/GameConfig.hpp"
#include "../starter/Load.hpp"
#include "../starter/data_path.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

using GameConfig::TILE_SIZE;

//Player art: one row of frames, each FRAME_W x FRAME_H, in the order down, up, left, right.
// (Placeholder until the sprite/animation system replaces it.)
static Load< Texture > player_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/dwarf.png"));
});
static constexpr int PLAYER_FRAME_W = 32;
static constexpr int PLAYER_FRAME_H = 32;

void Player::update(FrameInput const &input, float elapsed, World const &world) {
	auto direction = input.direction;
	//diagonal input shouldn't be faster than straight input
	if (glm::length(direction) > 1.0f) direction = glm::normalize(direction);

	moving = (direction != glm::vec2(0.0f));
	if (!moving) return;

	//face whichever way we're mostly going:
	if (std::abs(direction.x) > std::abs(direction.y)) {
		facing = glm::ivec2(direction.x > 0.0f ? 1 : -1, 0);
	} else {
		facing = glm::ivec2(0, direction.y > 0.0f ? 1 : -1);
	}

	//move one axis at a time, this is what lets you slide along walls instead of sticking to them when moving diagonally
	glm::vec2 step = direction * WALK_SPEED * elapsed;
	move_axis(0, step.x, world);
	move_axis(1, step.y, world);
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
    int frame = 0; //down
	if (facing.y < 0) frame = 1;      //up
	else if (facing.x < 0) frame = 2; //left
	else if (facing.x > 0) frame = 3; //right
	Rect src{float(frame * PLAYER_FRAME_W), 0.0f, float(PLAYER_FRAME_W), float(PLAYER_FRAME_H)};
	//round to whole pixels so the sprite never sits between pixels:
	glm::vec2 top_left = glm::round(position) - glm::vec2(PLAYER_FRAME_W * 0.5f, float(PLAYER_FRAME_H));
	batch.draw(*player_texture, src, top_left);
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

glm::ivec2 Player::facing_tile() const {
	//measure from the middle of the hitbox so "in front" matches what you see
	glm::vec2 middle = position - glm::vec2(0.0f, HITBOX_H * 0.5f);
	return Tilemap::tile_at_point(middle) + facing;
}

void Player::get_occupied(std::vector<glm::ivec2> *ans) const {
	glm::vec2 middle = position - glm::vec2(0.0f, HITBOX_H * 0.5f);
	
	ans->emplace_back(Tilemap::tile_at_point(middle));

}