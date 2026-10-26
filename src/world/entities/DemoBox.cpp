#include "DemoBox.hpp"
#include "../../starter/Load.hpp"
#include "../../starter/data_path.hpp"

#include <cmath>

static Load< Texture > box_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/player.png"));
});


void DemoBox::update(FrameInput const &input, float elapsed, Tilemap const &map) {
    return;
}

void DemoBox::draw(Sprites &batch) const {
    int frame = 0; //down
    Rect src{float(frame * 16), 0.0f, float(16), float(24)};
    //round to whole pixels so the sprite never sits between pixels:
    glm::vec2 top_left = glm::round(position) - glm::vec2(16 * 0.5f, float(24));
    batch.draw(*box_texture, src, top_left);

}

auto DemoBox::hitbox() const -> Rect {
    return Rect();
}