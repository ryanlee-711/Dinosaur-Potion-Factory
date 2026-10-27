#include "Chest.hpp"
#include "../../starter/Load.hpp"
#include "../../starter/data_path.hpp"

#include <cmath>

static Load< Texture > box_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/chest.png"));
});


void Chest::update(FrameInput const &input, float elapsed, Tilemap const &map) {
    return;
}

void Chest::draw(Sprites &batch) const {
    Rect src{0.0f, 0.0f, 32.0f, 16.0f};
    //round to whole pixels so the sprite never sits between pixels:
    glm::vec2 top_left = glm::round(position);
    batch.draw(*box_texture, src, top_left);

}

auto Chest::hitbox() const -> Rect {
    return Rect{position.x, position.y, 32.0f, 16.0f};
}

void Chest::get_occupied(std::vector<glm::ivec2> *ans) const {
    auto tile = Tilemap::tile_at_point(position + glm::vec2{GameConfig::TILE_SIZE / 2.0f});
	ans->emplace_back(tile);
    ans->emplace_back(tile + glm::ivec2{1, 0});
}

void Chest::on_interact(Context &ctx) {
    // should trigger inventory display through some mechanism
    ctx.flag = Context::InteractionType::OpenInventory;
}

auto Chest::get_inventory() -> Inventory* {
    return &inventory;
}