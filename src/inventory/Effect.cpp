#include "Effect.hpp"
#include "Item.hpp"

#include "../starter/Load.hpp"

#include <iostream>
#include <memory>
#include <unordered_map>

// ============================ EFFECTS ============================
// Every effect the player can have. Levels and durations are set per consumable below.

using EffectRegistry = std::unordered_map< effect_id_t, Effect >;

static Load< EffectRegistry > effect_registry(LoadTagDefault, []() -> EffectRegistry const * {
	auto effects = std::make_unique< EffectRegistry >();
	auto add = [&](effect_id_t const &id, std::string const &name, std::string const &description) {
		effects->emplace(id, Effect{id, name, description});
	};

	add("speed", "Swift", "Walk faster (more per level)");
	add("strength", "Strong", "Level 1: break small rocks. Level 2: break large boulders");
	add("float", "Float", "Levitate and fly pass obstacles.");
	add("warmth", "Warmth", "Survive the cold the of snowy peaks.");

	return effects.release();
});

// ========================== CONSUMABLES ==========================
// What each drinkable item gives: { effect, seconds, level }.
// Items not listed here can't be consumed

using ConsumableRegistry = std::unordered_map< item_id_t, Consumable >;

static Load< ConsumableRegistry > consumable_registry(LoadTagDefault, []() -> ConsumableRegistry const * {
	auto consumables = std::make_unique< ConsumableRegistry >();
	auto add = [&](item_id_t const &item, std::vector< EffectGrant > grants) {
		consumables->emplace(item, Consumable{item, grants});
	};

	// ---- cave I: ale lets you push the rocks ----
	add("drupe_drink", { {"strength", 60.0f, 1}, {"speed", 30.0f, 1} });
	add("drupe_brew", { {"speed", 30.0f, 1} });

	// ---- cave II: strength potion breaks large boulders ----
	add("sweet_brew", { {"strength", 120.0f, 1} });
	add("strength_potion", { {"strength", 60.0f, 2} });
	add("extended_strength_potion", { {"strength", 180.0f, 2} });

	// ---- mountainside: haste and float -----
	add("float_potion", { {"float", 15.0f, 1} });
	add("extended_float_potion", { {"float", 45.0f, 1} });

	add("warmth_potion", { {"warmth", 120.0f, 1} });
	add("extended_warmth_potion", { {"warmth", 360.0f, 1} });

	add("haste_potion", { {"speed", 90.0f, 2} });
	add("extended_haste_potion", { {"speed", 180.0f, 2} });

	add("hercules_potion", { {"strength", 180000.0f, 2} });
	add("godspeed_potion", { {"speed", 180000.0f, 2} });
	add("flame_potion", { {"warmth", 180000.0f, 2} });
	add("fly_potion", { {"float", 180000.0f, 2} });

	return consumables.release();
});

// ============================ LOOKUPS ============================

Effect const *Effect::get(effect_id_t const &id) {
	auto it = effect_registry->find(id);
	return it != effect_registry->end() ? &it->second : nullptr;
}

Consumable const *Consumable::get(item_id_t const &item) {
	if (item.empty()) return nullptr;
	auto it = consumable_registry->find(item);
	return it != consumable_registry->end() ? &it->second : nullptr;
}

int Consumable::validate() {
	int problems = 0;
	for (auto const &[item, consumable] : *consumable_registry) {
		if (Item::get(item) == nullptr) {
			std::cerr << "[effects] consumable '" << item << "' is not a registered item" << std::endl;
			problems += 1;
		}
		for (EffectGrant const &grant : consumable.grants) {
			if (Effect::get(grant.effect) == nullptr) {
				std::cerr << "[effects] consumable '" << item << "' gives unknown effect '" << grant.effect << "'" << std::endl;
				problems += 1;
			}
		}
	}
	if (problems == 0) std::cout << "[effects] all consumables check out." << std::endl;
	return problems;
}