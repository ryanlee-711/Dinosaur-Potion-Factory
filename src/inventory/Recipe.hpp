/* 
 * Recipe.hpp
 * recipe dataclass for automationing stuff.
 *
 */
#pragma once

 #include "Item.hpp"
 #include "Inventory.hpp"


 struct Recipe {
   using RecipeRegistry = std::unordered_map<item_id_t, std::vector<Recipe>>;

    item_id_t machine_id = "brewer";
    std::vector<ItemRecord> items_in { };
    std::vector<ItemRecord> items_out { };
    float time_cost = 5.0f;

    // methods
    static std::vector<Recipe> const * get_recipes(item_id_t machine_id);
 };