#include "TileMap.hpp"

#include "../main/GameConfig.hpp"
#include "../rendering/Sprites.hpp"
#include "../rendering/Texture.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <deque>
#include <fstream>
#include <sstream>
#include <stdexcept>


using GameConfig::TILE_SIZE;

const int32_t SOLID_MASK = 1 << 31;

Tilemap::Tilemap(std::string const &map_path): map_path(map_path) {
	std::ifstream file(map_path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot open map: " + map_path);
    }

    uint32_t width;
	uint32_t height;

	// header [width, height] (uint32_t)
    file.read(reinterpret_cast<char *>(&width), sizeof(width));
    file.read(reinterpret_cast<char *>(&height), sizeof(height));
	file.read(reinterpret_cast<char *>(&num_layers), sizeof(num_layers));
    if (!file) throw std::runtime_error("Cannot read map header");

    size = glm::uvec2(width, height);
	layer_visibilities = std::vector<float>{ };
	for (uint32_t layer = 0; layer < num_layers; ++layer) {
		tile_layers.emplace_back();
		tile_layers.back().resize(size_t(width) * height);
		file.read(
			reinterpret_cast<char *>(tile_layers.back().data()),
			tile_layers.back().size() * sizeof(int32_t)
		);

		layer_visibilities.emplace_back(1.0f);
		if (!file) {
			throw std::runtime_error("Cannot read map cells");
		}
	}
	file.close();
}

glm::vec2 Tilemap::size_px() const {
	return glm::vec2(size) * float(TILE_SIZE);
}

bool Tilemap::in_bounds(glm::ivec2 tile) const {
	return tile.x >= 0 && tile.y >= 0 && tile.x < int(size.x) && tile.y < int(size.y);
}

int32_t Tilemap::tile_at(glm::ivec2 tile, size_t layer) const {
	if (layer >= tile_layers.size()) {
		return -1;
	}
	return tile_layers[layer][size_t(tile.y) * size.x + tile.x];
}

bool Tilemap::is_solid(glm::ivec2 tile) const {
	if (!in_bounds(tile)) return true;
	if (tile_layers.empty()) {
		return false;
	}
	return tile_layers[0][size_t(tile.y) * size.x + tile.x] & SOLID_MASK;
}

bool Tilemap::overlaps_solid(Rect const &box) const {
	//shrink max by a hair so a box that exactly touches a tile edge doesn't count as overlapping
	constexpr float EPS = 0.001f;
	glm::ivec2 first = tile_at_point(box.min());
	glm::ivec2 last = tile_at_point(box.max() - glm::vec2(EPS));
	for (int ty = first.y; ty <= last.y; ++ty) {
		for (int tx = first.x; tx <= last.x; ++tx) {
			if (is_solid(glm::ivec2(tx, ty))) return true;
		}
	}
	return false;
}

glm::ivec2 Tilemap::tile_at_point(glm::vec2 world_px) {
	return glm::ivec2(glm::floor(world_px / float(TILE_SIZE)));
}

glm::vec2 Tilemap::tile_center(glm::ivec2 tile) {
	return (glm::vec2(tile) + 0.5f) * float(TILE_SIZE);
}

glm::vec2 Tilemap::tile_base(glm::ivec2 tile) {
	return glm::vec2((float(tile.x) + 0.5f) * float(TILE_SIZE), float(tile.y + 1) * float(TILE_SIZE));
}

Rect Tilemap::tile_rect(glm::ivec2 tile) {
	return Rect{float(tile.x * TILE_SIZE), float(tile.y * TILE_SIZE), float(TILE_SIZE), float(TILE_SIZE)};
}

void Tilemap::validate_tileset(Texture const &tileset) const {

}

void Tilemap::draw(Sprites &batch, Texture const &tileset_texture, glm::vec2 view_min, glm::vec2 view_max) const {
	uint32_t columns = std::max(1u, tileset_texture.size.x / TILE_SIZE);

	//only visit tiles that are on screen
	glm::ivec2 first = glm::max(tile_at_point(view_min), glm::ivec2(0));
	glm::ivec2 last = glm::min(tile_at_point(view_max - glm::vec2(0.001f)), glm::ivec2(size) - 1);

	for (int ty = first.y; ty <= last.y; ++ty) {
		for (int tx = first.x; tx <= last.x; ++tx) {
			for (size_t layer = 0; layer < tile_layers.size(); ++layer) {
				int32_t index = tile_layers[layer][size_t(ty) * size.x + tx] & (~SOLID_MASK);
				Rect src{
					float((index % columns) * TILE_SIZE),
					float((index / columns) * TILE_SIZE),
					float(TILE_SIZE), float(TILE_SIZE)
				};
				batch.draw(tileset_texture, src, glm::vec2(tx * TILE_SIZE, ty * TILE_SIZE), glm::u8vec4(static_cast<int>(layer_visibilities[layer] * 255.0f)));
			}
		}
	}
}

void Tilemap::save() {
	// Save this map; write it to file


	std::ofstream file(map_path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot write to: " + map_path);
    }

	// header [width, height] (uint32_t)
    file.write(reinterpret_cast<char *>(&size.x), 4UL);
    file.write(reinterpret_cast<char *>(&size.y), 4UL);
	file.write(reinterpret_cast<char *>(&num_layers), sizeof(num_layers));
    if (!file) throw std::runtime_error("Cannot read map header");

	for (uint32_t layer = 0; layer < num_layers; ++layer) {
		file.write(
			reinterpret_cast<char *>(tile_layers[layer].data()),
			tile_layers[layer].size() * sizeof(int32_t)
		);
	}
	file.close();
}