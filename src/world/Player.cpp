#include "Player.hpp"

#include "Tilemap.hpp"
#include "../main/GameConfig.hpp"

#include <cmath>

using GameConfig::TILE_SIZE;

void Player::update(glm::vec2 direction, float elapsed, Tilemap const &map) {
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
	move_axis(0, step.x, map);
	move_axis(1, step.y, map);
}

void Player::move_axis(int axis, float amount, Tilemap const &map) {
	if (amount == 0.0f) return;

	//if we're somehow already stuck inside a wall, let the player walk out of it
	bool was_stuck = map.overlaps_solid(hitbox());

	position[axis] += amount;
	if (was_stuck) return;

	Rect box = hitbox();
	if (!map.overlaps_solid(box)) return;

	//we walked into a solid tile, push back to exactly touch its edge
	constexpr float EPS = 0.001f;
	float lo = box.min()[axis];
	float hi = box.max()[axis];
	float T = float(TILE_SIZE);
	if (amount > 0.0f) {
		float wall = std::floor((hi - EPS) / T) * T;  //near edge of the tile we hit
		position[axis] -= (hi - wall);
	} else {
		float wall = (std::floor(lo / T) + 1.0f) * T; //far edge of the tile we hit
		position[axis] += (wall - lo);
	}
}

Rect Player::hitbox() const {
	return Rect{position.x - HITBOX_W * 0.5f, position.y - HITBOX_H, HITBOX_W, HITBOX_H};
}

glm::ivec2 Player::facing_tile() const {
	//measure from the middle of the hitbox so "in front" matches what you see
	glm::vec2 middle = position - glm::vec2(0.0f, HITBOX_H * 0.5f);
	return Tilemap::tile_at_point(middle) + facing;
}
