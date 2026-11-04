#include "ItemCursor.hpp"

#include <algorithm>

int ItemCursor::take(Inventory &inv, size_t slot, int amount) {
	if (amount <= 0 || inv.slot_empty(slot)) return 0;
	item_id_t const id = inv.slots[slot].first;
	if (!empty() && held.first != id) return 0; //holding something else

	int room = Inventory::MAX_STACK - std::max(held.second, 0);
	int taken = inv.extract_from_slot(slot, std::min(amount, room));
	if (taken <= 0) return 0;

	if (empty()) held = ItemRecord{id, 0};
	held.second += taken;
	return taken;
}

int ItemCursor::put(Inventory &inv, size_t slot, int amount) {
	if (amount <= 0 || empty()) return 0;
	int moved = inv.insert_into_slot(slot, held.first, std::min(amount, held.second));
	held.second -= moved;
	if (held.second <= 0) clear();
	return moved;
}

ItemCursor::Result ItemCursor::swap(Inventory &inv, size_t slot) {
	if (empty() || inv.slot_empty(slot)) return Result::Nothing;
	if (!inv.can_extract(slot)) return Result::CantExtract;
	if (!inv.can_insert(slot, held.first)) return Result::CantInsert;
	std::swap(inv.slots[slot], held);
	return Result::Moved;
}

ItemCursor::Result ItemCursor::drop(Inventory &inv, size_t slot) {
	if (empty() || slot >= inv.size()) return Result::Nothing;

	if (!inv.slot_empty(slot) && inv.slots[slot].first != held.first) {
		return swap(inv, slot);
	}
	if (!inv.can_insert(slot, held.first)) return Result::CantInsert;
	return put(inv, slot, held.second) > 0 ? Result::Moved : Result::Nothing;
}

ItemCursor::Result ItemCursor::click(Inventory &inv, size_t slot, bool shift) {
	if (slot >= inv.size()) return Result::Nothing;
	bool slot_has_items = !inv.slot_empty(slot);

	if (shift) {
		if (!empty()) return drop(inv, slot); //put the whole stack down
		if (!slot_has_items) return Result::Nothing;
		if (!inv.can_extract(slot)) return Result::CantExtract;
		return take(inv, slot, Inventory::MAX_STACK) > 0 ? Result::Moved : Result::Nothing;
	}

	if (slot_has_items) {
		//pick up one more
		if (!empty() && held.first != inv.slots[slot].first) {
			//holding something else
			return inv.can_insert(slot, held.first) ? Result::Nothing : Result::CantInsert;
		}
		if (!inv.can_extract(slot)) return Result::CantExtract;
		return take(inv, slot, 1) > 0 ? Result::Moved : Result::Nothing;
	}

	//put one down
	if (empty()) return Result::Nothing;
	if (!inv.can_insert(slot, held.first)) return Result::CantInsert;
	return put(inv, slot, 1) > 0 ? Result::Moved : Result::Nothing;
}

int ItemCursor::return_to(Inventory &inv) {
	if (empty()) return 0;
	int left = inv.add(held.first, held.second);
	held.second = left;
	if (held.second <= 0) clear();
	return left;
}
