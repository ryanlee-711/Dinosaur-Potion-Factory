#include "World.hpp"

#include "Entity.hpp"
#include "TileMap.hpp"

#include <limits>

std::optional<Tilemap::Area> World::area_at(glm::vec2 point) const
{
	for (size_t i = 0; i < map.area_rects.size(); ++i)
	{
		for (Rect const &rect : map.area_rects[i])
		{
			if (rect.contains(point))
				return static_cast<Tilemap::Area>(i);
		}
	}
	return std::nullopt;
}

float World::distance_to_area(Tilemap::Area area, glm::vec2 point) const
{
	float distance = std::numeric_limits<float>::infinity();
	for (Rect const &rect : map.area_rects.at(area))
	{
		float rect_distance = rect.distance_to(Rect{point.x, point.y});
		if (rect.contains(point))
		{
			rect_distance = -std::min({point.x - rect.x, rect.x + rect.w - point.x,
																 point.y - rect.y, rect.y + rect.h - point.y});
		}
		distance = std::min(distance, rect_distance);
	}
	return distance;
}

void World::solids_overlapping(Rect const &box, Entity const &mover, std::vector<Rect> &out) const
{
	// solid tiles: check every tile the box touches
	glm::ivec2 first = Tilemap::tile_at_point(box.min());
	glm::ivec2 last = Tilemap::tile_at_point(box.max());
	for (int ty = first.y; ty <= last.y; ++ty)
	{
		for (int tx = first.x; tx <= last.x; ++tx)
		{
			glm::ivec2 tile(tx, ty);
			if ((map.collision_layer_at(tile) & mover.collision_mask) == 0)
				continue;
			Rect r = Tilemap::tile_rect(tile);
			if (r.overlaps(box))
				out.emplace_back(r);
		}
	}

	// solid entities:
	for (auto const &entity : entities)
	{
		if (entity.get() == &mover)
			continue;
		if (entity->removed)
			continue;
		if (!entity->blocks(mover))
			continue;
		Rect r = entity->hitbox();
		if (r.overlaps(box))
			out.emplace_back(r);
	}
}

bool World::blocked(Rect const &box, Entity const &mover) const
{
	std::vector<Rect> hits;
	solids_overlapping(box, mover, hits);
	return !hits.empty();
}

auto World::entity_at(glm::ivec2 tile) const -> Entity *
{
	if (coord_to_entities.find(tile) != coord_to_entities.end())
	{
		auto &here = coord_to_entities.at(tile);
		if (here.size() == 0)
			return nullptr;
		return here[0];
	}
	return nullptr;
}
