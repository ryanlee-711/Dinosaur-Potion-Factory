#include "Boulder.hpp"
#include "../World.hpp"
#include "../TileMap.hpp"
#include "../../starter/Load.hpp"
#include "../../starter/data_path.hpp"

#include <cmath>
#include <array>

static Load< Texture > big_boulder_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/sprites/entities/big_boulder.png"));
});

static Load< Texture > small_boulder_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/sprites/entities/small_boulder.png"));
});

Boulder::Boulder(bool use_big): big(use_big) {
}

void Boulder::update(FrameInput const &input, float elapsed, World const &world) {
    if (rolling) {
        rolling = false;

        Rect hbox = hitbox();
        hbox.x += (Tilemap::tile_topleft(target_tile) - position).x;
        hbox.y += (Tilemap::tile_topleft(target_tile) - position).y;

        if (world.entity_at(target_tile) == nullptr && !world.blocked(hbox, *this)) {
            sprite_offset = position;
            position = Tilemap::tile_topleft(target_tile);
        }
    }
    // lerp towards target
    if (target_tile != glm::ivec2{0}) {
        sprite_offset = sprite_offset * (1.0f - 3.0f * elapsed) + position * (3.0f * elapsed);
    } else {
        sprite_offset = position;
    }
    
    if (glm::length(sprite_offset - position) < 0.1f) {
        sprite_offset = position;
    }
}

void Boulder::draw(Sprites &batch) const {
    if (big) {
        Rect src{0.0f, 0.0f, 32.0f, 32.0f};
        //round to whole pixels so the sprite never sits between pixels:
        glm::vec2 top_left = glm::round(sprite_offset) - glm::vec2(0.0f, 16.0f);
        batch.draw(*big_boulder_texture, src, top_left);
    } else {
        Rect src{0.0f, 0.0f, 16.0f, 16.0f};
        glm::vec2 top_left = glm::round(sprite_offset) - glm::vec2(0.0f, 0.0f);
        batch.draw(*small_boulder_texture, src, top_left);     
    }
}

auto Boulder::hitbox() const -> Rect {
    return big ? Rect{position.x, position.y - 16.0f, 32.0f, 32.0f} : Rect{position.x, position.y, 16.0f, 16.0f};
}

void Boulder::get_occupied(std::vector<glm::ivec2> *ans) const {
    auto tile = Tilemap::tile_at_point(position + glm::vec2{GameConfig::TILE_SIZE / 2.0f});
	ans->emplace_back(tile);
    if (big) {
        ans->emplace_back(tile + glm::ivec2{1, 0});
        ans->emplace_back(tile + glm::ivec2{1, -1});
        ans->emplace_back(tile + glm::ivec2{0, -1});
    }
}

void Boulder::on_interact(Context &ctx) {
    // only when powered by potion would it be movable
    if (ctx.player.effect_level("strength") <= 0 || big) {
        ctx.message = "Too heavy to move...";
        return;
    } else {
        target_tile = Tilemap::tile_at_point(position + glm::vec2{GameConfig::TILE_SIZE / 2.0f});
        if (ctx.player.position.x > position.x) {
            target_tile.x -= 1;
        } else {
            target_tile.x += 1;
        }
        rolling = true;
    }
}
