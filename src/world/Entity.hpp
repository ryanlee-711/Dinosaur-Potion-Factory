#pragma once

#include "../main/Rect.hpp"
#include "../rendering/Sprites.hpp"
#include "../starter/Mode.hpp"
#include "../main/GameConfig.hpp"

#include <glm/glm.hpp>


struct World;

struct FrameInput {
    struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	};

    glm::vec2 direction;
    Button interact;
};

struct Entity {
	glm::vec2 position = glm::vec2(0.0f);  // world pixels, see POSITION RULE above

	// Two entities block each other when (a.collision_mask & b.collision_mask) != 0.
	// Default 0x1 = solid. Set to 0 for things you can walk through
	uint32_t collision_mask = 0x1;

	virtual ~Entity() = default;

	virtual void update(FrameInput const &input, float elapsed, World const &world) = 0;
	virtual Rect hitbox() const = 0;
	virtual void draw(Sprites &batch) const = 0;

	bool blocks(Entity const &other) const {
		return (collision_mask & other.collision_mask) != 0;
	}
};