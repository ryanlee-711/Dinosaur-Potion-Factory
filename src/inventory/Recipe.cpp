#include "Recipe.hpp"

#include <iostream>
#include <string>
#include <memory>
#include <unordered_map>
#include <algorithm>
#include <fstream>
#include <sstream>
#include "../starter/Load.hpp"
#include "../starter/data_path.hpp"


using RecipeRegistry = Recipe::RecipeRegistry;

constexpr char const *RECIPE_PATH = "data/recipes.tsv";

static Load<RecipeRegistry> recipe_registry(LoadTagDefault,
	[]() -> RecipeRegistry const* {
		auto recipes = std::make_unique<RecipeRegistry>();

		auto add = [&](item_id_t const &machine_id, std::vector<ItemRecord> items_in, std::vector<ItemRecord> items_out, float time_cost, float probability) {
			Recipe recipe{ machine_id, items_in, items_out, time_cost, probability };

            if (recipes->find(machine_id) == recipes->end()) {
                recipes->emplace(machine_id, std::vector<Recipe>{ });
            }
			recipes->at(machine_id).emplace_back(recipe);
		};

		using ItemVector = std::vector<ItemRecord>;

        std::ifstream file(data_path(RECIPE_PATH));
        if (!file.is_open()) {
            throw std::runtime_error{"Recipe data file is not found"};
        }

        std::string line;
        item_id_t machine_id;
        item_id_t in_1, in_2, out_1, out_2;
        float time_cost, probability;
        int in_1_count, in_2_count, out_1_count, out_2_count;
        while (std::getline(file, line)) {
            std::istringstream ss(line);
            if (!(ss >> machine_id)) {
                continue;
            }
            if (machine_id == "brewer") {
                ss >> probability >> time_cost >> in_1 >> in_1_count >> in_2 >> in_2_count >> out_1 >> out_1_count;
                add(machine_id,
                    ItemVector{ItemRecord{in_1, in_1_count}, ItemRecord{in_2, in_2_count}},
                    ItemVector{ItemRecord{out_1, out_1_count}},
                    time_cost, probability);
            } else if (machine_id == "crusher") {
                ss >> probability >> time_cost >> in_1 >> in_1_count >> out_1 >> out_1_count;
                add(machine_id,
                    ItemVector{ItemRecord{in_1, in_1_count}},
                    ItemVector{ItemRecord{out_1, out_1_count}},
                    time_cost, probability);
            } else if (machine_id == "furnace") {
                ss >> probability >> time_cost >> in_1 >> in_1_count >> out_1 >> out_1_count;
                add(machine_id,
                    ItemVector{ItemRecord{in_1, in_1_count}},
                    ItemVector{ItemRecord{out_1, out_1_count}},
                    time_cost, probability);
            } else if (machine_id == "sieve") {
                ss >> probability >> time_cost >> in_1 >> in_1_count >> out_1 >> out_1_count;
                add(machine_id,
                    ItemVector{ItemRecord{in_1, in_1_count}},
                    ItemVector{ItemRecord{out_1, out_1_count}},
                    time_cost, probability);
            } else if (machine_id == "distiller") {
                ss >> probability >> time_cost >> in_1 >> in_1_count >> out_1 >> out_1_count >> out_2 >> out_2_count;
                add(machine_id,
                    ItemVector{ItemRecord{in_1, in_1_count}},
                    ItemVector{ItemRecord{out_1, out_1_count}, ItemRecord{out_2, out_2_count}},
                    time_cost, probability);
            } else if (machine_id == "fermenter") {
                ss >> probability >> time_cost >> in_1 >> in_1_count >> in_2 >> in_2_count >> out_1 >> out_1_count >> out_2 >> out_2_count;
                add(machine_id,
                    ItemVector{ItemRecord{in_1, in_1_count}, ItemRecord{in_2, in_2_count}},
                    ItemVector{ItemRecord{out_1, out_1_count}, ItemRecord{out_2, out_2_count}},
                    time_cost, probability);
            }
        }


        // Dragon crafting


        add("dragon",
            ItemVector{ ItemRecord{"copper_ore", 3}, ItemRecord{"lichen", 1} },
            ItemVector{ ItemRecord{"furnace", 1} },
            1.5f, 1.0f
        );

        add("dragon",
            ItemVector{ ItemRecord{"copper_ore", 3}, ItemRecord{"apple", 1} },
            ItemVector{ ItemRecord{"brewer", 1} },
            1.5f, 1.0f
        );

        add("dragon",
            ItemVector{ ItemRecord{"copper_ore", 3}, ItemRecord{"rock", 1} },
            ItemVector{ ItemRecord{"crusher", 2} },
            1.5f, 1.0f
        );

        add("dragon",
            ItemVector{ ItemRecord{"copper_ore", 1} },
            ItemVector{ ItemRecord{"pipe", 1} },
            0.5f, 1.0f
        );

        add("dragon",
            ItemVector{ ItemRecord{"rock", 3} },
            ItemVector{ ItemRecord{"chest", 1} },
            1.5f, 1.0f
        );

		return recipes.release();
	}
);


std::vector<Recipe> const * Recipe::get_recipes(item_id_t machine_id) {
    if (recipe_registry->find(machine_id) == recipe_registry->end()) return nullptr;
    return &recipe_registry->at(machine_id);
}

std::vector<Recipe const *> Recipe::all() {
    std::vector<item_id_t> machines;
    for (auto const &[machine_id, recipes] : *recipe_registry) machines.push_back(machine_id);
    std::sort(machines.begin(), machines.end());

    std::vector<Recipe const *> result;
    for (item_id_t const &machine_id : machines) {
        for (Recipe const &recipe : recipe_registry->at(machine_id)) result.push_back(&recipe);
    }
    return result;
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