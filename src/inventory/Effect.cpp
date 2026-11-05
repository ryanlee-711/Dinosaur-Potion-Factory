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
	add("ale", { {"strength", 60.0f, 1}, {"speed", 30.0f, 1} });
	add("apple_juice", { {"speed", 30.0f, 1} });
	add("sweet_cider", { {"speed", 60.0f, 1} });

	// ---- cave II: strength potion breaks large boulders ----
	add("strong_ale", { {"strength", 120.0f, 1} });
	add("strength_potion", { {"strength", 60.0f, 2} });
	add("potent_strength_potion", { {"strength", 120.0f, 2} });
	add("sweetened_potion", { {"speed", 45.0f, 2} });

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