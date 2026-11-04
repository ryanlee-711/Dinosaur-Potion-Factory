#include "ItemEntity.hpp"
#include "World.hpp"
#include "TileMap.hpp"
#include "../starter/Load.hpp"
#include "../starter/data_path.hpp"

#include <cmath>
#include <array>
#include <iostream>

/*
 * --- ItemEntity ---
 * class for item entities. Dropped on the ground.
 * Compare Stardew Valley.
 *
 */

float ABSORB_ACCEL_FACTOR = 85000.0f;
float ABSORB_MAX_SPEED = 30.0f;
float GRAVITATE_DISTANCE = 120.0f;
float FRICTION_FACTOR = 40.0f;
float VERTICAL_GRAVITY = 6.0f;

ItemEntity::ItemEntity(): inventory(1) {
    collision_mask = 0x0;
}

ItemEntity::ItemEntity(item_id_t item_id, int item_count): inventory(1) {
    inventory.add(item_id, item_count);
    vertical_velocity = -1.2f;
}

void ItemEntity::update(FrameInput const &input, float elapsed, World const &world) {
    // decrement absorb cooldown
    no_absorb_cooldown += elapsed;
    if (no_absorb_cooldown > 10.0f) no_absorb_cooldown -= 2.0f * 3.14159265358f;  // overflow prevention
    // move the item
    float dist = glm::length(world.player.position - position);

    if (dist < 4.0f) {
        ready_to_absorb = true;
    } else {
        // accelerating towards player
        if (dist < GRAVITATE_DISTANCE) {
            float accel = glm::pow(dist + 1e-6, -2.0f) * ABSORB_ACCEL_FACTOR;
            velocity += glm::normalize(world.player.position - position) * accel * elapsed;
        }
        // friction
        float speed = glm::length(velocity);
        if (speed > FRICTION_FACTOR * elapsed) {
            velocity = (speed - FRICTION_FACTOR * elapsed) * glm::normalize(velocity);
        } else {
            velocity = glm::vec2(0.0f);
        }

        velocity = glm::max(glm::min(velocity, 60.0f), -60.0f);
    }
    position += velocity * elapsed;

    // a decorative vertical speed/position...
    vertical_velocity += VERTICAL_GRAVITY * elapsed;
    height = std::min(height + vertical_velocity, 0.0f);
}

void ItemEntity::draw(Sprites &batch) const {
    if (item_id_t item_id = inventory.get_slot_id(0); item_id != "") {
        Item const *item = Item::get(item_id);
        Rect src{0.0f, 0.0f, 16.0f, 16.0f};
        //round to whole pixels so the sprite never sits between pixels:
        glm::vec2 top_left = glm::round(position + glm::vec2{-8.0f, -8.0f + height + glm::sin(no_absorb_cooldown * 2.0f)});
        batch.draw(*item->texture, src, top_left);
    }
}

auto ItemEntity::hitbox() const -> Rect {
    return Rect{position.x, position.y, 16.0f, 16.0f};
}

void ItemEntity::get_occupied(std::vector<glm::ivec2> *ans) const {
    // Item shouldn't occupy physical space ;-)
    return;
}

auto ItemEntity::get_inventory() -> Inventory* {
    return &inventory;
}

bool ItemEntity::is_absorbable() {
    return ready_to_absorb && no_absorb_cooldown > 0.5f;
}