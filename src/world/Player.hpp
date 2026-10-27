#pragma once

#include "../main/Rect.hpp"
#include "Entity.hpp"

#include <glm/glm.hpp>

struct World;

// The dwarf: movement, collision and drawing.
//
// 'position' is the point under the player's feet (bottom-center of the sprite),
struct Player : Entity {
public:
	glm::ivec2 facing = glm::ivec2(0, 1);  // last direction moved: (0,1)=down, (0,-1)=up, (+-1,0)=right/left
	bool moving = false;

	static constexpr float WALK_SPEED = 80.0f; // pixels per second (5 tiles/second)

	// Collision box around the feet. Smaller than the sprite on purpose so only the feet collide
	static constexpr float HITBOX_W = 14.0f;
	static constexpr float HITBOX_H = 14.0f;

	// How far in front of the feet the dwarf can reach to interact (pixels).
	static constexpr float REACH = 8.0f;

	// Moves the player by input.direction and stops at solid tiles and solid entities.
	void update(FrameInput const &input, float elapsed, World const &world) override;


	Rect hitbox() const override;
	void draw(Sprites &batch) const override;
	void get_occupied(std::vector<glm::ivec2> *ans) const override;

	// The area right in front of the feet, in the facing direction.
	// Whatever entity this overlaps is what pressing E interacts with.
	Rect interact_box() const;
	
	// The tile directly in front of the dwarf (for tile-based things later, like placing machines).
	glm::ivec2 facing_tile() const;

private:
	void move_axis(int axis, float amount, World const &world);
};