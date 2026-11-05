#include "Recipe.hpp"

#include <iostream>
#include <string>
#include <memory>
#include <unordered_map>
#include "../starter/Load.hpp"

using RecipeRegistry = Recipe::RecipeRegistry;


static Load<RecipeRegistry> recipe_registry(LoadTagDefault,
	[]() -> RecipeRegistry const* {
		auto recipes = std::make_unique<RecipeRegistry>();

		auto add = [&](item_id_t const &machine_id, std::vector<ItemRecord> items_in, std::vector<ItemRecord> items_out, float time_cost) {
			Recipe recipe{ machine_id, items_in, items_out, time_cost };

            if (recipes->find(machine_id) == recipes->end()) {
                recipes->emplace(machine_id, std::vector<Recipe>{ });
            }
			recipes->at(machine_id).emplace_back(recipe);
		};

		using ItemVector = std::vector<ItemRecord>;

        // -------------- APPLE -------------
        add("crusher",
            ItemVector{ ItemRecord{"apple", 1} },
            ItemVector{ ItemRecord{"crushed_apple", 1} },
            2.0f
        );

        add("crusher",
            ItemVector{ ItemRecord{"baked_apple", 1} },
            ItemVector{ ItemRecord{"baked_crushed_apple", 1} },
            2.0f
        );

        add("furnace",
            ItemVector{ ItemRecord{"apple", 1} },
            ItemVector{ ItemRecord{"baked_apple", 1} },
            8.0f
        );

        add("furnace",
            ItemVector{ ItemRecord{"crushed_apple", 1} },
            ItemVector{ ItemRecord{"apple_powder", 1} },
            4.0f
        );

        add("furnace",
            ItemVector{ ItemRecord{"baked_apple", 1} },
            ItemVector{ ItemRecord{"ash", 1} },
            8.0f
        );

        add("furnace",
            ItemVector{ ItemRecord{"apple_powder", 1} },
            ItemVector{ ItemRecord{"ash", 1} },
            4.0f
        );

        // ------- Apple potions ---------

        add("brewer",
            ItemVector{ ItemRecord{"water_bottle", 1}, ItemRecord{"crushed_apple", 3} },
            ItemVector{ ItemRecord{"ale", 1} },
            10.0f
        );

        add("brewer",
            ItemVector{ ItemRecord{"water_bottle", 1}, ItemRecord{"apple_powder", 3} },
            ItemVector{ ItemRecord{"apple_juice", 1} },
            10.0f
        );

        // --------------- LICHEN -----------------
        add("crusher",
            ItemVector{ ItemRecord{"lichen", 1} },
            ItemVector{ ItemRecord{"crushed_lichen", 1} },
            2.0f
        );

        add("crusher",
            ItemVector{ ItemRecord{"dried_lichen", 1} },
            ItemVector{ ItemRecord{"lichen_powder", 1} },
            2.0f
        );

        add("furnace",
            ItemVector{ ItemRecord{"lichen", 1} },
            ItemVector{ ItemRecord{"dried_lichen", 1} },
            8.0f
        );

        add("furnace",
            ItemVector{ ItemRecord{"crushed_lichen", 1} },
            ItemVector{ ItemRecord{"lichen_powder", 1} },
            4.0f
        );

        add("furnace",
            ItemVector{ ItemRecord{"dried_lichen", 1} },
            ItemVector{ ItemRecord{"ash", 1} },
            8.0f
        );

        add("furnace",
            ItemVector{ ItemRecord{"lichen_powder", 1} },
            ItemVector{ ItemRecord{"ash", 1} },
            4.0f
        );

        // --------------- LICHEN POTIONS ---------------
        add("brewer",
            ItemVector{ ItemRecord{"water_bottle", 1}, ItemRecord{"lichen_powder", 5} },
            ItemVector{ ItemRecord{"red_potion", 1} },
            20.0f
        );

        add("brewer",
            ItemVector{ ItemRecord{"red_potion", 1}, ItemRecord{"lichen_powder", 5} },
            ItemVector{ ItemRecord{"potent_red_potion", 1} },
            20.0f
        );

        add("brewer",
            ItemVector{ ItemRecord{"red_potion", 1}, ItemRecord{"crushed_apple", 3} },
            ItemVector{ ItemRecord{"strength_potion", 1} },
            10.0f
        );

        add("brewer",
            ItemVector{ ItemRecord{"potent_red_potion", 1}, ItemRecord{"crushed_apple", 3} },
            ItemVector{ ItemRecord{"potent_strength_potion", 1} },
            10.0f
        );

        // bad/hidden recipes
        add("brewer",
            ItemVector{ ItemRecord{"water_bottle", 1}, ItemRecord{"crushed_lichen", 5} },
            ItemVector{ ItemRecord{"muddy_red_potion", 1} },
            20.0f
        );

        add("brewer",
            ItemVector{ ItemRecord{"muddy_red_potion", 1}, ItemRecord{"crushed_apple", 3} },
            ItemVector{ ItemRecord{"strong_ale", 1} },
            10.0f
        );

        add("brewer",
            ItemVector{ ItemRecord{"red_potion", 1}, ItemRecord{"baked_crushed_apple", 3} },
            ItemVector{ ItemRecord{"sweetened_potion", 1} },
            10.0f
        );

		return recipes.release();
	}
);


std::vector<Recipe> const * Recipe::get_recipes(item_id_t machine_id) {
    if (recipe_registry->find(machine_id) == recipe_registry->end()) return nullptr;
    return &recipe_registry->at(machine_id);
}

// Which tags a machine's input slots accept
// Keep in sync with the SlotRules in each machine's constructor
static uint32_t input_tags_for(item_id_t const &machine_id) {
    using namespace ItemTag;
    if (machine_id == "crusher") return Crushable;
    if (machine_id == "furnace") return Burnable;
    if (machine_id == "brewer")  return Liquid | Potion | Brewable; // base slot (Liquid | Potion) | ingredient slot
    return None;
}

int Recipe::validate() {
    int problems = 0;
    auto warn = [&](Recipe const &recipe, std::string const &what) {
        std::cerr << "[recipes] " << recipe.machine_id << " recipe making '"
                  << (recipe.items_out.empty() ? std::string("?") : recipe.items_out[0].first)
                  << "': " << what << std::endl;
        problems += 1;
    };

    for (auto const &[machine_id, recipes] : *recipe_registry) {
        uint32_t accepted = input_tags_for(machine_id);
        for (Recipe const &recipe : recipes) {
            for (ItemRecord const &in : recipe.items_in) {
                if (Item::get(in.first) == nullptr) {
                    warn(recipe, "input '" + in.first + "' is not a registered item");
                } else if (accepted != 0 && (Item::tags_of(in.first) & accepted) == 0) {
                    warn(recipe, "input '" + in.first + "' doesn't have a tag the " + machine_id + "'s slots accept");
                }
            }
            for (ItemRecord const &out : recipe.items_out) {
                if (Item::get(out.first) == nullptr) {
                    warn(recipe, "output '" + out.first + "' is not a registered item");
                }
            }
        }
    }
    if (problems == 0) std::cout << "[recipes] all recipes check out." << std::endl;
    return problems;
}