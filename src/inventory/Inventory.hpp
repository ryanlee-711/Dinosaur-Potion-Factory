#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

typedef std::string item_id_t;

// One inventory slot: (item id, count). An empty slot has id "" and count 0.
using ItemRecord = std::pair< item_id_t, int >;

struct Inventory {
	static constexpr size_t DEFAULT_SLOTS = 10;
	static constexpr int MAX_STACK = 99; // max stack of an item

	// Rules for moving items into / out of one slot from OUTSIDE the inventory's owner:
	// the player's mouse and other machines (pipes, push_item). The owner itself ignores
	// them, e.g. a crusher writes into its own output slot with add_to_slot_range().
	struct SlotRule {
		bool can_insert = true;      // false = take-only slot (products, apple tree)
		bool can_extract = true;     // false = put-only slot
		uint32_t required_tags = 0;  // 0 = any item; else the item needs at least one of these ItemTag bits
	};

	explicit Inventory(size_t slot_count = DEFAULT_SLOTS);

	// ---------- Owner-side API (ignores slot rules) ----------

	// Adds up to 'count' of an item: tops up existing stacks of it first, then
	// fills empty slots left to right. Returns how many did NOT fit (0 = all added)
	int add(item_id_t const &id, int count);

	// Adds to specific slot range [begin, end). Returns how many did NOT fit.
	int add_to_slot_range(item_id_t const &id, int count, size_t begin, size_t end);

	// Removes exactly 'count' of an item, or nothing at all if there aren't enough.
	// Returns true if they were removed. Takes from the last stacks first.
	bool remove(item_id_t const &id, int count);

	int count(item_id_t const &id) const; // total held across all slots
	bool has(item_id_t const &id, int amount = 1) const { return count(id) >= amount; }
	int space_for(item_id_t const &id) const; // how many more of this item would fit
	bool empty() const;

	// ---------- Slot rules (player / pipe side) ----------

	void set_rule(size_t slot, SlotRule rule);
	SlotRule const &rule(size_t slot) const { return rules[slot]; }

	// Would this slot accept this item from outside? (checks can_insert and tags; ignores how full it is)
	bool can_insert(size_t slot, item_id_t const &id) const;
	bool can_extract(size_t slot) const;

	// Puts up to 'count' of an item into one slot, obeying its rule and the stack limit.
	// Returns how many were actually put in.
	int insert_into_slot(size_t slot, item_id_t const &id, int count);

	// Takes up to 'count' items out of one slot, obeying its rule.
	// Returns how many were actually taken (their id is whatever the slot held).
	int extract_from_slot(size_t slot, int count);

	// ---------- helpers ----------
	bool slot_empty(size_t slot) const { return slot >= size() || slots[slot].second <= 0; }

	item_id_t get_slot_id(int index) const {
		if (index < 0 || size_t(index) >= size()) {
			return "";
		}
		return slots[index].first;
	}
	int get_slot_count(int index) const {
		if (index < 0 || size_t(index) >= size()) {
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
	std::vector< SlotRule > rules; // one per slot; default = anything goes
};
