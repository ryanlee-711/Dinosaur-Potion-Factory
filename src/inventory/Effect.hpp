#pragma once

#include "Inventory.hpp" //item_id_t

#include <string>
#include <vector>

// Effects: timed statuses on the player (speed, strength, ...), and what each consumable gives.
//
// The tables only describe effects.
//What an effect *does* is code that asks the player about it

typedef std::string effect_id_t;

struct Effect {
	effect_id_t id;
	std::string name;         // shown in the effects list on screen
	std::string description;

	// nullptr if no effect has this id
	static Effect const *get(effect_id_t const &id);
};

// One thing a consumable does: give 'effect' at 'level' for 'seconds'.
struct EffectGrant {
	effect_id_t effect;
	float seconds = 0.0f;
	int level = 1;
};

// An item you can drink / eat with F, and the effects it gives.
struct Consumable {
	item_id_t item;
	std::vector< EffectGrant > grants;

	// nullptr = this item can't be consumed
	static Consumable const *get(item_id_t const &item);

	// Prints a warning for every consumable whose item or effect id doesn't exist.
	// Call once after assets are loaded. Returns the number of problems.
	static int validate();
};