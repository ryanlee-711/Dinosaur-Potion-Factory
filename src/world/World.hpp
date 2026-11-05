#pragma once

#include "../main/Rect.hpp"
#include "../world/Player.hpp"
#include "../world/TileMap.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

struct Tilemap;
struct Entity;

// A read-only view of everything an entity can bump into this frame:
// (Read only in the sense that the vector can't be modified. But entities can!)
// the tilemap plus the other entities
struct World {
	Tilemap const &map;
	std::vector< std::unique_ptr< Entity > > const &entities;
	Player const &player;
	std::unordered_map<glm::ivec2, std::vector<Entity *>, TileHash> const &coord_to_entities;
	//spawned entities
	std::vector<std::unique_ptr<Entity>> *spawned = nullptr;

	// Appends to 'out' every solid thing 'box' overlaps that 'mover' can't walk through
	void solids_overlapping(Rect const &box, Entity const &mover, std::vector< Rect > &out) const;

	// True if 'box' overlaps anything solid for 'mover'.
	bool blocked(Rect const &box, Entity const &mover) const;

	// Find one entity at the given location.
	Entity * entity_at(glm::ivec2 tile) const;
};