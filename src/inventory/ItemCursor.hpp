#pragma once

#include "Inventory.hpp"

// Controls (PlayMode calls click() on every left mouse press over a slot):
//   click        empty-handed      -> pick up ONE item. This is the origin spot
//   click        the origin slot   -> pick up ONE more
//   click        any other slot    -> put ONE down if slot is empty or hold the same item
//   shift-click  empty-handed      -> pick up the WHOLE stack (that slot becomes the origin).
//   shift-click  while holding     -> put the WHOLE held stack down
struct ItemCursor {
	ItemRecord held{"", 0};

	// Where the held stack was picked up from
	Inventory const *origin = nullptr;
	size_t origin_slot = 0;

	enum class Result {
		Nothing,      // nothing to do
		Moved,        // items moved
		CantInsert,   // the slot doesn't accept the held item
		CantExtract,  // the slot is put-only
	};

	bool empty() const { return held.second <= 0; }
	void clear() { held = ItemRecord{"", 0}; origin = nullptr; origin_slot = 0; }
	bool is_origin(Inventory const &inv, size_t slot) const { return !empty() && origin == &inv && origin_slot == slot; }

	// A click on a slot
	Result click(Inventory &inv, size_t slot, bool shift);

	// Both return how many items moved.
	int take(Inventory &inv, size_t slot, int amount); // slot -> cursor
	int put(Inventory &inv, size_t slot, int amount);  // cursor -> slot

	// Put down the whole held stack
	Result put_all(Inventory &inv, size_t slot);

	// Exchange the held stack with a slot holding a different item
	Result swap(Inventory &inv, size_t slot);

	// Puts everything held back into inventory
	// Returns how many items did NOT fit and those stay on the cursor.
	int return_to(Inventory &inv);
};
