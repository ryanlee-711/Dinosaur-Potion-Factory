#pragma once

#include "../main/Rect.hpp"
#include "../world/Player.hpp"

#include <memory>
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

	// Appends to 'out' every solid thing 'box' overlaps that 'mover' can't walk through
	void solids_overlapping(Rect const &box, Entity const &mover, std::vector< Rect > &out) const;

	// True if 'box' overlaps anything solid for 'mover'.
	bool blocked(Rect const &box, Entity const &mover) const;
};