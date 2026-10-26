#include <string>
#include <unordered_map>

#include "../rendering/Texture.hpp"
#include "../main/Rect.hpp"

typedef std::string item_id_t;

struct Item {
    using ItemRegistry = std::unordered_map<std::string, Item>;
    item_id_t id;
    std::string name;
    std::string description;
    std::unique_ptr<Texture> texture;
    Rect tex_rect;

    static const Item* get(std::string id);
private:
    static const ItemRegistry registry;
};