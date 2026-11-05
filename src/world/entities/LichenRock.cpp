#include "LichenRock.hpp"
#include "../World.hpp"
#include "../TileMap.hpp"
#include "../../starter/Load.hpp"
#include "../../starter/data_path.hpp"

#include <cmath>

static Load< Texture > box_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/test/lichen_rock.png"));
});

void LichenRock::update(FrameInput const &input, float elapsed, World const &world) {
    // regrow one lichen every REGROW_COOLDOWN seconds, but only while it's not full
    if (lichen >= MAX_LICHEN) {
        regrow_timer = REGROW_COOLDOWN;
        return;
    }
    regrow_timer -= elapsed;
    if (regrow_timer <= 0.0f) {
        regrow_timer = REGROW_COOLDOWN;
        lichen += 1;
    }
}

void LichenRock::on_interact(Context &ctx) {
    if (lichen <= 0) {
        ctx.message = "No lichen yet";
        return;
    }
    // pick everything that fits in the bag; whatever doesn't fit stays on the rock
    int left = ctx.player_inventory.add("lichen", lichen);
    lichen = left;
    if (left > 0) ctx.message = "Your bag is full";
}

void LichenRock::draw(Sprites &batch) const {
    // frame 1 = lichen on it, frame 0 = bare rock
    Rect src{lichen > 0 ? 32.0f : 0.0f, 0.0f, 32.0f, 32.0f};
    //round to whole pixels so the sprite never sits between pixels:
    glm::vec2 top_left = glm::round(position) - glm::vec2(0.0f, 16.0f);
    batch.draw(*box_texture, src, top_left);
}

auto LichenRock::hitbox() const -> Rect {
    return Rect{position.x, position.y, 32.0f, 16.0f};
}

void LichenRock::get_occupied(std::vector<glm::ivec2> *ans) const {
    auto tile = Tilemap::tile_at_point(position + glm::vec2{GameConfig::TILE_SIZE / 2.0f});
	ans->emplace_back(tile);
    ans->emplace_back(tile + glm::ivec2{1, 0});
}