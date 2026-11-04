#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <memory>

#include "../rendering/Texture.hpp"
#include "../main/Rect.hpp"
#include "Inventory.hpp" //item_id_t, ItemRecord, Inventory

// What kind of thing an item is. An item can have several tags (bitwise OR them).
// Inventory slots use these to decide what they accept (see Inventory::SlotRule),
// e.g. a crusher's input slot only takes Crushable items.
// To add a new tag: add the next bit here, then tag items in Item.cpp.
namespace ItemTag {
	enum : uint32_t {
		None      = 0,
		Crushable = 1u << 0, // can go into a crusher
		Burnable  = 1u << 1, // can go into a baker/furnace (planned)
		Brewable  = 1u << 2, // brewing ingredient (cauldron ingredient slot)
		Liquid    = 1u << 3, // brewing base (cauldron water slot)
		Potion    = 1u << 4, // a finished drink/potion
		Placeable = 1u << 5, // places a machine in the world
		Tool      = 1u << 6, // pickaxe etc.
	};
}

struct Item {
	using ItemRegistry = std::unordered_map<std::string, Item>;
	item_id_t id;
	std::string name;
	std::string description;
	std::unique_ptr<Texture> texture;
	uint32_t tags = ItemTag::None;
	Rect tex_rect;

	bool has_tag(uint32_t tag) const { return (tags & tag) != 0; }

	static const Item* get(std::string id);

	// Tags of an item id; 0 if the id is empty or unknown.
	static uint32_t tags_of(item_id_t const &id);
private:
	static const ItemRegistry registry;
};
