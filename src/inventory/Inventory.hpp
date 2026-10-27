#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

typedef std::string item_id_t;

// One inventory slot: (item id, count). An empty slot has id "" and count 0.
using ItemRecord = std::pair< item_id_t, int >;

struct Inventory {
	static constexpr size_t DEFAULT_SLOTS = 10;
	static constexpr int MAX_STACK = 99; // max stack of an item

	explicit Inventory(size_t slot_count = DEFAULT_SLOTS);

	// Adds up to 'count' of an item: tops up existing stacks of it first, then
	// fills empty slots left to right. Returns how many did NOT fit (0 = all added)
	int add(item_id_t const &id, int count);

	// Adds to specific slot range [begin, end)
	int add_to_slot_range(item_id_t const &id, int count, size_t begin, size_t end);

	// Removes exactly 'count' of an item, or nothing at all if there aren't enough.
	// Returns true if they were removed. Takes from the last stacks first.
	bool remove(item_id_t const &id, int count);

	int count(item_id_t const &id) const; // total held across all slots
	bool has(item_id_t const &id, int amount = 1) const { return count(id) >= amount; }
	int space_for(item_id_t const &id) const; // how many more of this item would fit
	bool empty() const;

	// some other helpers
	item_id_t get_slot_id(int index) const {
		if (index < 0 || index > size()) {
			return "";
		}
		return slots[index].first;
	}
	int get_slot_count(int index) const {
		if (index < 0 || index > size()) {
			return 0;
		}
		return slots[index].second;
	}

	int space_for_in_slot_range(item_id_t const &id, size_t begin, size_t end) const; // how many more of this item would fit

	size_t size() const { return slots.size(); } // number of slots (full or empty)
	ItemRecord const &operator[](size_t i) const { return slots[i]; }

	std::vector< ItemRecord >::const_iterator begin() const { return slots.begin(); }
	std::vector< ItemRecord >::const_iterator end() const { return slots.end(); }

// private:   // for now
	std::vector< ItemRecord > slots;
};