#include "Item.hpp"

#include <sstream>
#include <fstream>
#include <string>
#include <memory>
#include <unordered_map>
#include "../rendering/Texture.hpp"
#include "../starter/Load.hpp"
#include "../starter/data_path.hpp"


using ItemRegistry = Item::ItemRegistry;

static Load<ItemRegistry> item_registry(LoadTagDefault,
	[]() -> ItemRegistry const* {
		auto items = std::make_unique<ItemRegistry>();

		auto add = [&](item_id_t const &id, std::string const &name, std::string const &description,
		               std::string const &texture_path, uint32_t tags) {
			Item item;
			item.id = id;
			item.name = name;
			item.description = description;
			item.texture = std::make_unique<Texture>(data_path(texture_path));
			item.tags = tags;
			items->emplace(id, std::move(item));
		};

		using namespace ItemTag;

		// Placeholder sprite for items that don't have art yet
		std::string const MISSING = "assets/sprites/items/missing_no.png";

		// Tags decide which machine slots accept an item, so they follow the recipes in Recipe.cpp:
		//   Crushable = crusher input, Burnable = furnace input,
		//   Brewable = brewer ingredient, Liquid = brewer base (water and potions),
		//   Potion = something you drink (see the effects table).

		// ---- apple line ----
		add("apple", "Apple", "Grows in the cave, somehow", "assets/sprites/items/ingredients/apple.png", Crushable | Burnable);
		add("apple_powder", "Apple Powder", "Crushed apple", "assets/sprites/items/ingredients/apple_powder.png", Brewable | Burnable);
		add("baked_apple", "Baked Apple", "Warm and soft", MISSING, Crushable | Burnable);
		add("baked_apple_powder", "Baked Apple Powder", "Sweet and smoky", MISSING, Brewable | Burnable);

		// ---- lichen line ----
		add("lichen", "Red Lichen", "Something magical", MISSING, Crushable | Burnable);
		add("lichen_powder", "Lichen Powder", "Crushed raw lichen", MISSING, Brewable | Burnable);
		add("baked_lichen", "Baked Lichen", "Crispy", MISSING, Crushable | Burnable);
		add("baked_lichen_powder", "Red Powder", "Glows a little", MISSING, Brewable | Burnable);

		// ---- leftovers (burnt too long) ----
		add("goo", "Goo", "Overcooked. Useless", MISSING, None);
		add("ash", "Ash", "Burnt to a crisp", MISSING, None);

		// ---- liquids and potions ----
		// Potions are also Liquid so they fit the brewer's base slot (red potion -> strength potion, ...)
		add("water_bottle", "Water Bottle", "Just water", "assets/sprites/items/potions/water_bottle.png", Liquid);
		add("ale", "Ale", "Non-alchohol drink", "assets/sprites/items/potions/ale.png", Potion | Liquid);
		add("apple_juice", "Apple Juice", "Healthy", "assets/sprites/items/potions/apple_juice.png", Potion | Liquid);
		add("sweet_cider", "Sweet Cider", "Made with baked apples", MISSING, Potion | Liquid);
		add("strong_ale", "Strong Ale", "Lichen gives it a kick", MISSING, Potion | Liquid);
		add("red_potion", "Red Potion", "Base for stronger potions", MISSING, Potion | Liquid);
		add("strong_red_potion", "Strong Red Potion", "Extra concentrated", MISSING, Potion | Liquid);
		add("weak_red_potion", "Weak Red Potion", "Made from raw lichen", MISSING, Potion | Liquid);
		add("strength_potion", "Strength Potion", "Better than ale", MISSING, Potion | Liquid);
		add("strong_strength_potion", "Strong Strength Potion", "Much better than ale", MISSING, Potion | Liquid);
		add("sweetened_potion", "Sweetened Potion", "Red potion, sweetened", MISSING, Potion | Liquid);

		// placeables
		add("pipe", "Pipe", "N/A", "assets/sprites/items/placeables/pipe_icon.png", Placeable);
		add("chest", "Chest", "N/A", "assets/sprites/items/placeables/chest_icon.png", Placeable);
		add("crusher", "Crusher", "N/A", "assets/sprites/items/placeables/crusher_icon.png", Placeable);
		add("brewer", "brewer", "N/A", "assets/sprites/items/placeables/brewer_icon.png", Placeable);

		// tools
		add("pickaxe", "Pickaxe", "A sturdy pick", "assets/sprites/items/pickaxe.png", Tool);

		return items.release();
	}
);

const Item* Item::get(std::string id) {
	if (!item_registry) return nullptr;
	auto it = item_registry->find(id);
	return it != item_registry->end() ? &it->second : nullptr;
}

uint32_t Item::tags_of(item_id_t const &id) {
	if (id.empty()) return ItemTag::None;
	Item const *item = get(id);
	return item ? item->tags : ItemTag::None;
}