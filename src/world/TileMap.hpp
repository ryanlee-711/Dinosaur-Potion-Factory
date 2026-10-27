#pragma once

#include "../main/Rect.hpp"

#include <glm/glm.hpp>

#include <cstdint>
#include <string>
#include <vector>

struct Sprites;
struct Texture;

// A grid of tiles loaded from a text .map file
//
// Coordinates:
//   - tile coordinates: (column, row), (0,0) is the top-left tile
//   - world coordinates: art pixels, (0,0) is the top-left corner of the map, y grows down
//   - tile (tx, ty) covers world pixels [tx*TILE_SIZE, (tx+1)*TILE_SIZE)
struct Tilemap {

	// Loads the map
	explicit Tilemap(std::string const &map_path);

	glm::uvec2 size = glm::uvec2(0);   // in tiles
	uint32_t num_layers = 0;
	std::string tileset_path;          // relative to the dist/ folder

	// each layer is row-major; each entry indexes tile_types
	std::vector<std::vector< int32_t >> tile_layers;

	// --- queries ---
	glm::vec2 size_px() const;
	bool in_bounds(glm::ivec2 tile) const;
	int32_t tile_at(glm::ivec2 tile, size_t layer) const; // tile must be in bounds
	bool is_solid(glm::ivec2 tile) const;            // outside the map counts as solid
	bool overlaps_solid(Rect const &box) const;      // does a world-space box touch any solid tile?

	static glm::ivec2 tile_at_point(glm::vec2 world_px);
	static glm::vec2 tile_center(glm::ivec2 tile);

	// Throws if any tile type points past the end of the tileset image
	void validate_tileset(Texture const &tileset) const;

	// Draws only the tiles inside [view_min, view_max)
	void draw(Sprites &batch, Texture const &tileset, glm::vec2 view_min, glm::vec2 view_max) const;
};
