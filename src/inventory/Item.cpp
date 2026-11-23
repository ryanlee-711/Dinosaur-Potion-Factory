#include "Item.hpp"

#include <ranges>
#include <sstream>
#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <unordered_map>
#include "../rendering/Texture.hpp"
#include "../starter/Load.hpp"
#include "../starter/data_path.hpp"


constexpr char const *ITEMS_PATH = "data/items.tsv";

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

		std::ifstream file(data_path(ITEMS_PATH));

		if (!file.is_open()) {
			throw std::runtime_error{"Couldn't open ITEMS_PATH"};
		}
		// for every line in the file, read and parse
		std::string line;
		while (std::getline(file, line)) {
			if (!line.empty() && line.back() == '\r') line.pop_back(); //file saved with Windows line endings
			if (line.empty()) continue;
			std::vector<std::string> parts { };
			for (auto part : line | std::views::split('\t')) {
				parts.emplace_back(part.begin(), part.end());
			}
			// id, name, description, sprite, then up to 8 tag columns.
			// Missing tag columns count as empty (editors often trim the trailing tabs of a row).
			if (parts.size() < 4 || parts.size() > 12) {
				std::cout << "warning: skipping invalid item line (needs 4 to 12 columns): " << line << '\n';
				continue;
			}
			uint32_t tag = 0x0;
			for (size_t idx = 0; idx < 8 && idx + 4 < parts.size(); ++idx) {
				if (parts[idx + 4] == "1") {
					tag |= 1u << idx;
				}
			}
			add(parts[0], parts[1], parts[2], parts[3], tag);

		}



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