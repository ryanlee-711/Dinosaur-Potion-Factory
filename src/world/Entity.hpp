#pragma once

#include "../main/Rect.hpp"
#include "../rendering/Sprites.hpp"
#include "../starter/Mode.hpp"

#include <glm/glm.hpp>


struct Tilemap;

struct FrameInput {
    struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	};

    glm::vec2 direction;
    Button interact;
};

// Entity - ABC for in-game objects that could hold data.
// We can later switch to ECS; for the time being, inheritance is easier to implement.
struct Entity {
	glm::vec2 position = glm::vec2(0.0f);  // world pixels
    uint32_t collision_mask = 0x1;

    virtual ~Entity() = default;

	virtual void update(FrameInput const &input, float elapsed, Tilemap const &map) = 0;
	virtual Rect hitbox() const = 0;
    virtual void draw(Sprites &batch) const = 0;
};