#pragma once

#include "Inventory.hpp"

// Controls (PlayMode calls click() on every left mouse press over a slot):
//   click        empty-handed      -> pick up ONE item. That slot becomes the stack's "origin".
//   click        the origin slot   -> pick up ONE more.
//   click        any other slot    -> put ONE down there (slot must be empty or hold the same item).
//   shift-click  empty-handed      -> pick up the WHOLE stack (that slot becomes the origin).
//   shift-click  while holding     -> put the WHOLE held stack down
//                                     (merges into a matching stack, swaps with a different item).
struct ItemCursor {
	ItemRecord held{"", 0};

	// Where the held stack was picked up from. Only ever compared, never dereferenced,
	// so it's harmless if that inventory has since been closed.
	Inventory const *origin = nullptr;
	size_t origin_slot = 0;

	enum class Result {
		Nothing,      // nothing to do (e.g. clicked an empty slot empty-handed)
		Moved,        // items moved
		CantInsert,   // the slot doesn't accept the held item
		CantExtract,  // the slot is put-only
	};

	bool empty() const { return held.second <= 0; }
	void clear() { held = ItemRecord{"", 0}; origin = nullptr; origin_slot = 0; }
	bool is_origin(Inventory const &inv, size_t slot) const { return !empty() && origin == &inv && origin_slot == slot; }

	// A left click on a slot (see the table above).
	Result click(Inventory &inv, size_t slot, bool shift);

	// Low-level moves. Both return how many items moved.
	int take(Inventory &inv, size_t slot, int amount); // slot -> cursor (makes the slot the origin)
	int put(Inventory &inv, size_t slot, int amount);  // cursor -> slot

	// Put down the whole held stack (merge into a matching/empty slot, swap with a different item).
	Result put_all(Inventory &inv, size_t slot);

	// Exchange the held stack with a slot holding a different item (if both directions are allowed).
	Result swap(Inventory &inv, size_t slot);

	// Puts everything held back into 'inv' (owner-side add, ignores rules).
	// Returns how many items did NOT fit; those stay on the cursor.
	int return_to(Inventory &inv);
};
