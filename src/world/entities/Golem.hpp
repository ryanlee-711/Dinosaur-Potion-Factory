#pragma once

#include "../../main/Rect.hpp"
#include "../Entity.hpp"
#include "../../inventory/Effect.hpp"
#include "../Components.hpp"

#include <glm/glm.hpp>
#include <map>

struct World;


// 'position' is the point under the golem's feet (bottom-center of the sprite),
struct Golem : public Entity, public IInventoryHolder, public IBreakable {
public:
	Golem();
	glm::ivec2 facing = glm::ivec2(0, 1);  // last direction moved: (0,1)=down, (0,-1)=up, (+-1,0)=right/left
	bool moving = false;
	float dust_distance = 0.0f;
	static constexpr float DUST_STEP = 10.0f;

	static constexpr float WALK_SPEED = 20.0f; // pixels per second (5 tiles/second)
	static constexpr float SPRINT_MULTIPLIER = 1.5f; // holding Shift multiplies walking speed by this
	static constexpr float SPEED_PER_LEVEL = 0.5f; // +50% speed per level

	// Collision box around the feet. Smaller than the sprite on purpose so only the feet collide
	static constexpr float HITBOX_W = 14.0f;
	static constexpr float HITBOX_H = 14.0f;

	// How far in front of the feet the dwarf can reach to interact (pixels).
	static constexpr float REACH = 8.0f;

	// Moves the golem by input.direction and stops at solid tiles and solid entities.
	void update(FrameInput const &input, float elapsed, World const &world) override;


	Rect hitbox() const override;
	void draw(Sprites &batch) const override;
	void get_occupied(std::vector<glm::ivec2> *ans) const override;

	std::string save() const override;
    void load(std::string line) override;

	// IInventoryHolder
	auto get_inventory() -> Inventory* override {return &inventory;};

	item_id_t get_drop_id() override { return ""; }

	// Golem
	uint32_t fuelbox_uid() const { return fuel_box; }
	void bind_fuelbox(uint32_t uid) { fuel_box = uid; initialized = true; }

	// The area right in front of the feet, in the facing direction.
	// Whatever entity this overlaps is what pressing E interacts with.
	Rect interact_box() const;

	// The tile directly in front of the dwarf (for tile-based things later, like placing machines).
	glm::ivec2 facing_tile() const;

	ItemRecord held_item = ItemRecord{"", 0};

	void reload_targets(World const &world);

private:
	void move_axis(int axis, float amount, World const &world);

	Inventory inventory;

	// The fuel box is a special chest that provides the Golem with fuels.
	// When there is no fuel box, the golem may no longer work.
	uint32_t fuel_box = 0;

	// Target entity: could be a producer, or a chest.
	// The golem cycles between producer and chest. It could only subscribe to one chest and one producer.
	// In its lifetime, th cycles
	uint32_t binded_chest = 0;
	uint32_t binded_producer = 0;
	uint32_t target_entity = 0;

	float state_change_timeout = 0.0f;
	float work_cooldown = 0.0f;
	float absorb_item_cooldown = 0.0f;

	float animation_timer = 0.0f;

	bool initialized = false;
};
