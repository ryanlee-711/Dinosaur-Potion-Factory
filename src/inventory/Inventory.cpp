#include "Inventory.hpp"

#include <algorithm>

Inventory::Inventory(size_t slot_count) : slots(slot_count, ItemRecord{"", 0}) {
}

int Inventory::add(item_id_t const &id, int count) {
	if (id.empty() || count <= 0) return 0;

	//first top up stacks that already hold this item:
	for (auto &[slot_id, slot_count] : slots) {
		if (count == 0) break;
		if (slot_id != id || slot_count >= MAX_STACK) continue;
		int moved = std::min(count, MAX_STACK - slot_count);
		slot_count += moved;
		count -= moved;
	}

	//then start new stacks in empty slots:
	for (auto &[slot_id, slot_count] : slots) {
		if (count == 0) break;
		if (slot_count != 0) continue;
		int moved = std::min(count, MAX_STACK);
		slot_id = id;
		slot_count = moved;
		count -= moved;
	}

	return count; //whatever is left didn't fit
}

bool Inventory::remove(item_id_t const &id, int count) {
	if (count <= 0) return true;
	if (this->count(id) < count) return false; //all or nothing

	//take from the last stacks first, so the first slots (hotbar) stay put
	for (auto it = slots.rbegin(); it != slots.rend() && count > 0; ++it) {
		auto &[slot_id, slot_count] = *it;
		if (slot_id != id) continue;
		int taken = std::min(count, slot_count);
		slot_count -= taken;
		count -= taken;
		if (slot_count == 0) slot_id.clear(); //slot is empty again
	}
	return true;
}

int Inventory::count(item_id_t const &id) const {
	int total = 0;
	for (auto const &[slot_id, slot_count] : slots) {
		if (slot_id == id) total += slot_count;
	}
	return total;
}

int Inventory::space_for(item_id_t const &id) const {
	int space = 0;
	for (auto const &[slot_id, slot_count] : slots) {
		if (slot_count == 0) space += MAX_STACK;
		else if (slot_id == id) space += MAX_STACK - slot_count;
	}
	return space;
}

bool Inventory::empty() const {
	for (auto const &[slot_id, slot_count] : slots) {
		if (slot_count != 0) return false;
	}
	return true;
}