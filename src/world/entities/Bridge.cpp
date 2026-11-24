#include "Bridge.hpp"
#include "Bridge.hpp"
#include "../World.hpp"
#include "../TileMap.hpp"
#include "../../starter/data_path.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>

void Bridge::draw(Sprites &batch) const {
	if (bridge_num == 0) batch.draw(**def->texture, Rect{64.0f * float(!repaired), 0.0f, 64.0f, 128.0f}, position);
	else  batch.draw(**def->texture, Rect{0.0f, 64.0f * float(!repaired), 128.0f, 64.0f}, position);
}

auto Bridge::hitbox() const -> Rect {
	if (bridge_num == 0)
		return Rect{position.x, position.y, 64.0f, 128.0f};
	else
		return Rect{position.x, position.y, 128.0f, 64.0f};
}

void Bridge::get_occupied(std::vector<glm::ivec2> *ans) const {
	// temp
	ans->emplace_back(Tilemap::tile_at_point(position));
}