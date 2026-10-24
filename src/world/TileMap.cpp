#include "Tilemap.hpp"

#include "../main/GameConfig.hpp"
#include "../rendering/Sprites.hpp"
#include "../rendering/Texture.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>

using GameConfig::TILE_SIZE;

namespace {
	std::string trim(std::string const &s) {
		size_t a = s.find_first_not_of(" \t\r");
		if (a == std::string::npos) return "";
		size_t b = s.find_last_not_of(" \t\r");
		return s.substr(a, b - a + 1);
	}
}

Tilemap::Tilemap(std::string const &map_path) {
	std::ifstream file(map_path, std::ios::binary);
	if (!file) throw std::runtime_error("Tilemap: couldn't open '" + map_path + "'.");

	size_t line_number = 0;
	auto fail = [&](std::string const &message) {
		throw std::runtime_error(map_path + ":" + std::to_string(line_number) + ": " + message);
	};

	//which tile_types entry each character maps to (-1 = not defined):
	std::array< int, 256 > symbol_to_type;
	symbol_to_type.fill(-1);

	bool in_map_section = false;
	bool have_spawn = false;
	std::vector< std::string > rows;

	std::string line;
	while (std::getline(file, line)) {
		++line_number;
		if (!line.empty() && line.back() == '\r') line.pop_back(); //windows line endings

		if (in_map_section) {
			if (trim(line).empty()) continue; //ignore blank lines
			rows.emplace_back(line);
			if (rows.back().size() != rows.front().size()) {
				fail("map row is " + std::to_string(rows.back().size()) + " characters wide but the first row is "
					+ std::to_string(rows.front().size()) + ". All rows must be the same width.");
			}
			continue;
		}

		//header section: strip comments (';' to end of line)
		std::string content = trim(line.substr(0, line.find(';')));
		if (content.empty()) continue;

		std::istringstream words(content);
		std::string keyword;
		words >> keyword;

		if (keyword == "tileset") {
			if (!(words >> tileset_path)) fail("expected: tileset <path>");
		} else if (keyword == "tile") {
			std::string symbol, solidity;
			TileType type;
			if (!(words >> symbol >> type.atlas_index >> solidity)) {
				fail("expected: tile <character> <tileset index> <walk|solid> [name]");
			}
			if (symbol.size() != 1) fail("tile character must be a single character, got '" + symbol + "'");
			if (solidity != "walk" && solidity != "solid") fail("expected 'walk' or 'solid', got '" + solidity + "'");
			type.symbol = symbol[0];
			type.solid = (solidity == "solid");
			words >> type.name; //optional
			if (symbol_to_type[uint8_t(type.symbol)] != -1) fail(std::string("tile '") + type.symbol + "' is defined twice");
			symbol_to_type[uint8_t(type.symbol)] = int(tile_types.size());
			tile_types.emplace_back(type);
		} else if (keyword == "spawn") {
			if (!(words >> spawn_tile.x >> spawn_tile.y)) fail("expected: spawn <column> <row>");
			have_spawn = true;
		} else if (keyword == "map") {
			in_map_section = true;
		} else {
			fail("unknown keyword '" + keyword + "'");
		}
	}

	if (tileset_path.empty()) throw std::runtime_error(map_path + ": missing 'tileset' line.");
	if (rows.empty()) throw std::runtime_error(map_path + ": missing 'map' section (or it is empty).");
	if (!have_spawn) throw std::runtime_error(map_path + ": missing 'spawn' line.");

	size = glm::uvec2(rows.front().size(), rows.size());
	tiles.resize(size_t(size.x) * size.y);
	for (uint32_t y = 0; y < size.y; ++y) {
		for (uint32_t x = 0; x < size.x; ++x) {
			char c = rows[y][x];
			int type = symbol_to_type[uint8_t(c)];
			if (type == -1) {
				throw std::runtime_error(map_path + ": map row " + std::to_string(y) + ", column " + std::to_string(x)
					+ " uses '" + std::string(1, c) + "', which has no 'tile' line.");
			}
			tiles[size_t(y) * size.x + x] = uint16_t(type);
		}
	}

	if (!in_bounds(glm::ivec2(spawn_tile))) throw std::runtime_error(map_path + ": spawn is outside the map.");
	if (is_solid(glm::ivec2(spawn_tile))) throw std::runtime_error(map_path + ": spawn is on a solid tile.");
}

glm::vec2 Tilemap::size_px() const {
	return glm::vec2(size) * float(TILE_SIZE);
}

bool Tilemap::in_bounds(glm::ivec2 tile) const {
	return tile.x >= 0 && tile.y >= 0 && tile.x < int(size.x) && tile.y < int(size.y);
}

Tilemap::TileType const &Tilemap::type_at(glm::ivec2 tile) const {
	return tile_types[tiles[size_t(tile.y) * size.x + tile.x]];
}

bool Tilemap::is_solid(glm::ivec2 tile) const {
	if (!in_bounds(tile)) return true;
	return type_at(tile).solid;
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

void Tilemap::validate_tileset(Texture const &tileset) const {
	uint32_t columns = tileset.size.x / TILE_SIZE;
	uint32_t count = columns * (tileset.size.y / TILE_SIZE);
	for (auto const &type : tile_types) {
		if (type.atlas_index >= count) {
			throw std::runtime_error("Tile '" + std::string(1, type.symbol) + "' uses tileset index "
				+ std::to_string(type.atlas_index) + ", but '" + tileset_path + "' only has "
				+ std::to_string(count) + " tiles.");
		}
	}
}

void Tilemap::draw(Sprites &batch, Texture const &tileset, glm::vec2 view_min, glm::vec2 view_max) const {
	uint32_t columns = std::max(1u, tileset.size.x / TILE_SIZE);

	//only visit tiles that are on screen
	glm::ivec2 first = glm::max(tile_at_point(view_min), glm::ivec2(0));
	glm::ivec2 last = glm::min(tile_at_point(view_max - glm::vec2(0.001f)), glm::ivec2(size) - 1);

	for (int ty = first.y; ty <= last.y; ++ty) {
		for (int tx = first.x; tx <= last.x; ++tx) {
			uint32_t index = type_at(glm::ivec2(tx, ty)).atlas_index;
			Rect src{
				float((index % columns) * TILE_SIZE),
				float((index / columns) * TILE_SIZE),
				float(TILE_SIZE), float(TILE_SIZE)
			};
			batch.draw(tileset, src, glm::vec2(tx * TILE_SIZE, ty * TILE_SIZE));
		}
	}
}
