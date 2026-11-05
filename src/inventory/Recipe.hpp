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

    // Checks every recipe against the item table and prints a warning for each problem:
    //   - an item id that isn't registered in Item.cpp (typo, or the item was never added)
    //   - an input its machine would reject (missing the tag that machine's slots require)
    // Call once after assets are loaded. Returns the number of problems found.
    static int validate();
 };