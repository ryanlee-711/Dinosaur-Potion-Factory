#include "Item.hpp"

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

        items->emplace("apple", Item{
            "apple",
            "Apple",
            "N/A",
            std::make_unique<Texture>(data_path("assets/test/apple.png"))
        });

        items->emplace("apple_powder", Item{
            "apple_powder",
            "Apple Powder",
            "N/A",
            std::make_unique<Texture>(data_path("assets/test/apple_powder.png"))
        });

        return items.release();
    }
);

const Item* Item::get(std::string id) {
    if (!item_registry) return nullptr;
    auto it = item_registry->find(id);
    return it != item_registry->end() ? &it->second : nullptr;
}
