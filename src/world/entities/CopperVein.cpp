#include "CopperVein.hpp"
#include "../World.hpp"
#include "../TileMap.hpp"
#include "../../starter/Load.hpp"
#include "../../starter/data_path.hpp"

#include <cmath>

static Load< Texture > box_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/sprites/entities/copper_vein.png"));
});

void CopperVein::update(FrameInput const &input, float elapsed, World const &world) {
    // regrow one ore every REGROW_COOLDOWN seconds, but only while it's not full
    if (ores >= MAX_ORES) {
        regrow_timer = REGROW_COOLDOWN;
        return;
    }
    regrow_timer -= elapsed;
    if (regrow_timer <= 0.0f) {
        regrow_timer = REGROW_COOLDOWN;
        ores += 1;
    }
}

item_id_t CopperVein::mine() {
    if (ores <= 0) return ""; //mined out: wait for it to regrow
    ores -= 1;
    return "copper_ore";
}

void CopperVein::draw(Sprites &batch) const {
    // frame 1 = has ore, frame 0 = mined out
    Rect src{ores > 0 ? 32.0f : 0.0f, 0.0f, 32.0f, 32.0f};
    //round to whole pixels so the sprite never sits between pixels:
    glm::vec2 top_left = glm::round(position) - glm::vec2(0.0f, 16.0f);
    batch.draw(*box_texture, src, top_left);
}

auto CopperVein::hitbox() const -> Rect {
    return Rect{position.x, position.y, 32.0f, 16.0f};
}

void CopperVein::get_occupied(std::vector<glm::ivec2> *ans) const {
    auto tile = Tilemap::tile_at_point(position + glm::vec2{GameConfig::TILE_SIZE / 2.0f});
	ans->emplace_back(tile);
    ans->emplace_back(tile + glm::ivec2{1, 0});
}