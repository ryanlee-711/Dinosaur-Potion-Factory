#include "Inventory.hpp"
#include "Item.hpp"

#include <algorithm>

Inventory::Inventory(size_t slot_count) : slots(slot_count, ItemRecord{"", 0}), rules(slot_count) {
}

int Inventory::add(item_id_t const &id, int count) {
	return add_to_slot_range(id, count, 0, slots.size());
}

int Inventory::add_to_slot_range(item_id_t const &id, int count, size_t begin, size_t end) {
	if (id.empty() || count <= 0) return 0;
	end = std::min(end, slots.size());

	for (size_t i = begin; i < end && count > 0; ++i) {
		auto &[slot_id, slot_count] = slots[i];
		if (slot_count <= 0 || slot_id != id || slot_count >= MAX_STACK) continue;
		int moved = std::min(count, MAX_STACK - slot_count);
		slot_count += moved;
		count -= moved;
	}

	for (size_t i = begin; i < end && count > 0; ++i) {
		auto &[slot_id, slot_count] = slots[i];
		if (slot_count > 0) continue;
		int moved = std::min(count, MAX_STACK);
		slot_id = id;
		slot_count = moved;
		count -= moved;
	}

	return count; //whatever is left didn't fit
}

bool Inventory::remove(item_id_t const &id, int count) {
	if (count <= 0) return true;
	if (this->count(id) < count) return false;

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
	return space_for_in_slot_range(id, 0, slots.size());
}

bool Inventory::empty() const {
	for (auto const &[slot_id, slot_count] : slots) {
		if (slot_count != 0) return false;
	}
	return true;
}

int Inventory::space_for_in_slot_range(item_id_t const &id, size_t begin, size_t end) const {
	int space = 0;
	for (size_t idx = begin; idx < std::min(end, size()); ++idx) {
		auto const &[slot_id, slot_count] = slots[idx];
		auto item = Item::get(id);
		if (item == nullptr) continue;
		if (rules[idx].required_tags != 0 && (Item::tags_of(id) & rules[idx].required_tags) == 0) continue;
		if (slot_count <= 0) space += MAX_STACK;
		else if (slot_id == id) space += MAX_STACK - slot_count;
	}
	return space;
}

// ---------- slot rules ----------

void Inventory::set_rule(size_t slot, SlotRule rule) {
	if (slot >= rules.size()) return;
	rules[slot] = rule;
}

bool Inventory::can_insert(size_t slot, item_id_t const &id) const {
	if (slot >= size() || id.empty()) return false;
	SlotRule const &r = rules[slot];
	if (!r.can_insert) return false;
	if (r.required_tags != 0 && (Item::tags_of(id) & r.required_tags) == 0) return false;
	return true;
}

bool Inventory::can_extract(size_t slot) const {
	if (slot >= size()) return false;
	return rules[slot].can_extract;
}

int Inventory::insert_into_slot(size_t slot, item_id_t const &id, int count) {
	if (count <= 0 || !can_insert(slot, id)) return 0;
	auto &[slot_id, slot_count] = slots[slot];
	if (slot_count > 0 && slot_id != id) return 0; //holds something else
	int moved = std::min(count, MAX_STACK - std::max(slot_count, 0));
	if (moved <= 0) return 0;
	if (slot_count <= 0) {
		slot_id = id;
		slot_count = 0;
	}
	slot_count += moved;
	return moved;
}

int Inventory::extract_from_slot(size_t slot, int count) {
	if (count <= 0 || !can_extract(slot) || slot_empty(slot)) return 0;
	auto &[slot_id, slot_count] = slots[slot];
	int taken = std::min(count, slot_count);
	slot_count -= taken;
	if (slot_count <= 0) {
		slot_count = 0;
		slot_id.clear();
	}
	return taken;
}
