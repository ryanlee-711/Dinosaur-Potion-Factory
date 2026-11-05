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

		// ingredients
		add("apple", "Apple", "", "assets/sprites/items/ingredients/apple.png", Crushable | Burnable);
		add("crushed_apple", "Crushed Apple", "", "assets/sprites/items/ingredients/crushed_apple.png", Brewable | Burnable);
		add("apple_powder", "Apple Powder", "", "assets/sprites/items/ingredients/crushed_apple.png", Brewable | Burnable);
		add("baked_apple", "Baked Apple", "", "assets/sprites/items/ingredients/baked_apple.png", Crushable | Burnable);

		add("lichen", "Lichen", "", "assets/sprites/items/ingredients/lichen.png", Crushable | Burnable);
		add("crushed_lichen", "Crushed Lichen", "", "assets/sprites/items/ingredients/crushed_lichen.png", Brewable | Burnable);
		add("lichen_powder", "Lichen Powder", "", "assets/sprites/items/ingredients/crushed_lichen.png", Brewable | Burnable);
		add("baked_lichen", "Baked Lichen", "", "assets/sprites/items/ingredients/baked_lichen.png", Crushable | Burnable);

		add("ash", "Ash", "N/A", "assets/sprites/items/ingredients/ash.png", None);

		// drinks
		add("water_bottle", "Water Bottle", "Just water", "assets/sprites/items/potions/water_bottle.png", Liquid);
		add("ale", "Ale", "Non-alchohol drink", "assets/sprites/items/potions/ale.png", Potion);
		add("potent_ale", "Strong Ale", "", "assets/sprites/items/potions/potent_ale.png", Potion);
		add("apple_juice", "Apple Juice", "Healthy", "assets/sprites/items/potions/apple_juice.png", Potion);

		add("red_potion", "Red Potion", "", "assets/sprites/items/potions/potent_red_potion.png", Potion);
		add("potent_red_potion", "Potent Red Potion", "", "assets/sprites/items/potions/potent_red_potion.png", Potion);
		add("muddy_red_potion", "Muddy Red Potion", "", "assets/sprites/items/potions/muddy_red_potion.png", Potion);

		add("strength_potion", "Strength Potion", "", "assets/sprites/items/potions/strength_potion.png", Potion);
		add("potent_strength_potion", "Potent Strength Potion", "", "assets/sprites/items/potions/potent_strength_potion.png", Potion);

		// placeables
		add("pipe", "Pipe", "", "assets/sprites/items/placeables/pipe_icon.png", Placeable);
		add("chest", "Chest", "", "assets/sprites/items/placeables/chest_icon.png", Placeable);
		add("crusher", "Crusher", "", "assets/sprites/items/placeables/crusher_icon.png", Placeable);
		add("brewer", "brewer", "", "assets/sprites/items/placeables/brewer_icon.png", Placeable);

		// tools
		add("empty_bottle", "Bottle", "", "assets/sprites/items/potions/empty_bottle.png", Liquid);
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
