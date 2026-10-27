#pragma once

#include "../main/Rect.hpp"
#include "Entity.hpp"

#include <glm/glm.hpp>

struct Tilemap;

// Handles movement and collision, drawing lives in PlayMode for now
//
// 'position' is the point under the player (bottom-center of the sprite)
struct Player : Entity {
public:
	glm::vec2 position = glm::vec2(0.0f);  // world pixels
	glm::ivec2 facing = glm::ivec2(0, 1);  // last direction moved: (0,1)=down, (0,-1)=up, (+-1,0)=right/left
	bool moving = false;

	static constexpr float WALK_SPEED = 80.0f; // pixels per second (5 tiles/second)

	// Collision box around the feet. Smaller than the sprite on purpose so only the feet collide
	static constexpr float HITBOX_W = 10.0f;
	static constexpr float HITBOX_H = 6.0f;

	// direction: from input, each axis in [-1, 1]. Moves the player and stops at solid tiles.
	void update(FrameInput const &input, float elapsed, Tilemap const &map) override;

	Rect hitbox() const override;
	void draw(Sprites &batch) const override;
	void get_occupied(std::vector<glm::ivec2> *ans) const override;

	// The tile directly in front of the dwarf (for interacting with things later).
	glm::ivec2 facing_tile() const;

private:
	void move_axis(int axis, float amount, Tilemap const &map);
};
