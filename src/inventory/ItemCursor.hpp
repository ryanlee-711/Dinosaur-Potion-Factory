#pragma once

#include "Inventory.hpp"

// Controls (see PlayMode for how mouse events map onto these):
//   click        on a slot  -> pick up ONE more item.
//                              If the slot is empty and you're holding something, put ONE down.
//   shift-click  on a slot  -> pick up the WHOLE stack.
//                              If the slot is empty put the WHOLE held stack down
//   drag + release on a slot -> put the WHOLE held stack down there.
struct ItemCursor {
	ItemRecord held{"", 0};

	enum class Result {
		Nothing,      // nothing to do
		Moved,        // items moved
		CantInsert,   // the slot doesn't accept the held item
		CantExtract,  // the slot is put-only
	};

	bool empty() const {
		return held.second <= 0;
	}
	void clear() {
		held = ItemRecord{"", 0};
	}

	// A click
	Result click(Inventory &inv, size_t slot, bool shift);

	// Releasing a drag
	Result drop(Inventory &inv, size_t slot);

	// How many items moved.
	int take(Inventory &inv, size_t slot, int amount); // slot -> cursor
	int put(Inventory &inv, size_t slot, int amount);  // cursor -> slot

	// Exchange the held stack with a different item
	Result swap(Inventory &inv, size_t slot);

	// Puts everything held back into inventory
	int return_to(Inventory &inv);
};
