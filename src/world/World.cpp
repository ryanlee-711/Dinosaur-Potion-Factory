#include "World.hpp"

#include "Entity.hpp"
#include "TileMap.hpp"

void World::solids_overlapping(Rect const &box, Entity const &mover, std::vector< Rect > &out) const {
	//solid tiles: check every tile the box touches
	glm::ivec2 first = Tilemap::tile_at_point(box.min());
	glm::ivec2 last = Tilemap::tile_at_point(box.max());
	for (int ty = first.y; ty <= last.y; ++ty) {
		for (int tx = first.x; tx <= last.x; ++tx) {
			glm::ivec2 tile(tx, ty);
			if (!map.is_solid(tile)) continue;
			Rect r = Tilemap::tile_rect(tile);
			if (r.overlaps(box)) out.emplace_back(r);
		}
	}

	//solid entities:
	for (auto const &entity : entities) {
		if (entity.get() == &mover) continue;
		if (entity->removed) continue;
		if (!entity->blocks(mover)) continue;
		Rect r = entity->hitbox();
		if (r.overlaps(box)) out.emplace_back(r);
	}
}

bool World::blocked(Rect const &box, Entity const &mover) const {
	std::vector< Rect > hits;
	solids_overlapping(box, mover, hits);
	return !hits.empty();
}