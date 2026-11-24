#pragma once

#include "../main/Rect.hpp"
#include "../rendering/Sprites.hpp"
#include "../starter/Mode.hpp"
#include "../main/GameConfig.hpp"

#include <glm/glm.hpp>
#include <memory>
#include <vector>


struct World;

struct FrameInput {
    struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	};

    glm::vec2 direction;
    bool sprint = false;   // Shift held and not in inventory
    Button interact, quicksave, flip;
    Button numkeys[10];
};

struct Entity {
	glm::vec2 position = glm::vec2(0.0f);  // world pixels, see POSITION RULE above

	// Two entities block each other when (a.collision_mask & b.collision_mask) != 0.
	// Default 0x1 = solid. Set to 0 for things you can walk through
	// 0b0001 = WALL, never walk through
	// 0b0010 = WATER, can have siphon and be flown over
	// 0b0100 = CLIFF, can be flown over
	// 0b1000 = ENTITY, can be flown over
	uint32_t collision_layer = 0b1000;
	uint32_t collision_mask = 0b1111;

	// A unique ID for this object, randomly generated in uninitialized; must be unique.
	uint32_t uid = next_uid++;

	// Set to true to delete this entity (picked up, harvested away, destroyed by a potion...).
	// PlayMode erases removed entities once per frame, after everything has updated
	bool removed = false;

	virtual ~Entity() = default;

	virtual void update(FrameInput const &input, float elapsed, World const &world) = 0;
	virtual Rect hitbox() const = 0;
	virtual void draw(Sprites &batch) const = 0;
	virtual void get_occupied(std::vector<glm::ivec2> *ans) const = 0;

	virtual std::string save() const = 0;
    virtual void load(std::string line) = 0;

	bool blocks(Entity const &other) const {
		return (collision_layer & other.collision_mask) != 0;
	}

	// For resetting UID counter
	static void reset_uid_counter(uint32_t new_uid) {
		next_uid = std::max(new_uid + 1, next_uid); 
	}
private:
    inline static uint32_t next_uid = 1;
};