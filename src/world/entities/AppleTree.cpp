#include "AppleTree.hpp"
#include "../World.hpp"
#include "../TileMap.hpp"
#include "../../starter/Load.hpp"
#include "../../starter/data_path.hpp"

#include <cmath>
#include <array>

static Load< Texture > box_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/test/apple_tree.png"));
});

AppleTree::AppleTree() : inventory(1) {

};

void AppleTree::update(FrameInput const &input, float elapsed, World const &world) {
    regrow_timer -= elapsed;
    if (regrow_timer <= 0.0f) {
        regrow_timer = REGROW_COOLDOWN;
        inventory.add("apple", 1);
    } 

    return;
}

void AppleTree::draw(Sprites &batch) const {
    Rect src{inventory.count("apple") ? 32.0f : 0.0f, 0.0f, 32.0f, 48.0f};
    //round to whole pixels so the sprite never sits between pixels:
    glm::vec2 top_left = glm::round(position) - glm::vec2(0.0f, 32.0f);
    batch.draw(*box_texture, src, top_left);

}

auto AppleTree::hitbox() const -> Rect {
    return Rect{position.x, position.y, 32.0f, 16.0f};
}

void AppleTree::get_occupied(std::vector<glm::ivec2> *ans) const {
    auto tile = Tilemap::tile_at_point(position + glm::vec2{GameConfig::TILE_SIZE / 2.0f});
	ans->emplace_back(tile);
    ans->emplace_back(tile + glm::ivec2{1, 0});
}

void AppleTree::on_interact(Context &ctx) {
    // should trigger inventory display through some mechanism
    ctx.open_container = &inventory;
}

auto AppleTree::get_inventory() -> Inventory* {
    return &inventory;
}
