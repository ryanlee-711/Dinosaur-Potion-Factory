#pragma once

#include "../main/Rect.hpp"
#include "../rendering/Sprites.hpp"
#include "../starter/Mode.hpp"
#include "../main/GameConfig.hpp"

#include <glm/glm.hpp>


struct Tilemap;

struct FrameInput {
    struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	};

    glm::vec2 direction;
    Button interact, tab;
    Button numkeys[10];
};

// Entity - ABC for in-game objects that could hold data.
// We can later switch to ECS; for the time being, inheritance is easier to implement.
struct Entity {
	glm::vec2 position = glm::vec2(0.0f);  // world pixels
    uint32_t collision_mask = 0x1;

    virtual ~Entity() = default;

	virtual void update(FrameInput const &input, float elapsed, Tilemap const &map, std::vector<std::unique_ptr<Entity>> const &entities) = 0;
	virtual Rect hitbox() const = 0;
    virtual void draw(Sprites &batch) const = 0;
    virtual void get_occupied(std::vector<glm::ivec2> *ans) const = 0;
};

inline auto world_to_grid(glm::vec2 world_pos) -> glm::ivec2 {
    world_pos /= GameConfig::TILE_SIZE;
    return glm::ivec2(static_cast<int>(world_pos.x), static_cast<int>(world_pos.y));
}

inline auto entity_at(std::vector<std::unique_ptr<Entity>> const &entities, glm::ivec2 tile) -> Entity* {
    for (auto const &entity : entities) {
        std::vector<glm::ivec2> ans;
        entity->get_occupied(&ans);
        for (glm::ivec2 &candidate : ans) {
            if (candidate == tile) {
                return entity.get();
            }
        }
    }
    return nullptr;
}