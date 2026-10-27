#include "DemoBox.hpp"
#include "../../main/GameConfig.hpp"
#include "../../starter/Load.hpp"
#include "../../starter/data_path.hpp"

#include <cmath>

static Load< Texture > box_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/test/box.png"));
});

//sprite size:
static constexpr float SPRITE_W = 16.0f;
static constexpr float SPRITE_H = 24.0f;
//collision + interaction size:
static constexpr float FOOT_W = float(GameConfig::TILE_SIZE);
static constexpr float FOOT_H = float(GameConfig::TILE_SIZE);


void DemoBox::update(FrameInput const &input, float elapsed, World const &world) {
    return;
}

void DemoBox::draw(Sprites &batch) const {
    Rect src{0.0f, 0.0f, SPRITE_W, SPRITE_H};
    //round to whole pixels so the sprite never sits between pixels:
    glm::vec2 top_left = glm::round(position) - glm::vec2(SPRITE_W * 0.5f, SPRITE_H);
    batch.draw(*box_texture, src, top_left);
}

auto DemoBox::hitbox() const -> Rect {
    return Rect{position.x - FOOT_W * 0.5f, position.y - FOOT_H, FOOT_W, FOOT_H};
}

void DemoBox::on_interact(Context &ctx) {
    // should trigger inventory display through some mechanism
    ctx.flag = Context::InteractionType::OpenInventory;
}

auto DemoBox::get_inventory() -> Inventory* {
    return &inventory;
}