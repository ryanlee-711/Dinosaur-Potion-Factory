#include "Recipe.hpp"

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
            ItemVector{ ItemRecord{"baked_crushed_apple", 1} },
            4.0f
        );

        add("furnace", 
            ItemVector{ ItemRecord{"baked_apple", 1} },
            ItemVector{ ItemRecord{"goo", 1} },
            8.0f
        );

        add("furnace", 
            ItemVector{ ItemRecord{"baked_crushed_apple", 1} },
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
            ItemVector{ ItemRecord{"water_bottle", 1}, ItemRecord{"baked_crushed_apple", 3} },
            ItemVector{ ItemRecord{"apple_drink", 1} },
            10.0f
        );

        // --------------- LICHEN -----------------
        add("crusher", 
            ItemVector{ ItemRecord{"lichen", 1} },
            ItemVector{ ItemRecord{"lichen_powder", 1} },
            2.0f
        );

        add("crusher", 
            ItemVector{ ItemRecord{"baked_lichen", 1} },
            ItemVector{ ItemRecord{"baked_lichen_powder", 1} },
            2.0f
        );

        add("furnace", 
            ItemVector{ ItemRecord{"lichen", 1} },
            ItemVector{ ItemRecord{"baked_lichen", 1} },
            8.0f
        );

        add("furnace", 
            ItemVector{ ItemRecord{"lichen_powder", 1} },
            ItemVector{ ItemRecord{"baked_lichen_powder", 1} },
            4.0f
        );

        add("furnace", 
            ItemVector{ ItemRecord{"baked_lichen", 1} },
            ItemVector{ ItemRecord{"goo", 1} },
            8.0f
        );

        add("furnace", 
            ItemVector{ ItemRecord{"baked_lichen_powder", 1} },
            ItemVector{ ItemRecord{"ash", 1} },
            4.0f
        );

        // --------------- LICHEN POTIONS ---------------
        add("brewer", 
            ItemVector{ ItemRecord{"water_bottle", 1}, ItemRecord{"baked_lichen_powder", 5} },
            ItemVector{ ItemRecord{"red_potion", 1} },
            20.0f
        );

        add("brewer", 
            ItemVector{ ItemRecord{"red_potion", 1}, ItemRecord{"baked_lichen_powder", 5} },
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
            ItemVector{ ItemRecord{"water_bottle", 1}, ItemRecord{"lichen_powder", 5} },
            ItemVector{ ItemRecord{"muddy_red_potion", 1} },
            20.0f
        );

        add("brewer", 
            ItemVector{ ItemRecord{"muddy_red_potion", 1}, ItemRecord{"crushed_apple", 3} },
            ItemVector{ ItemRecord{"potent_ale", 1} },
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