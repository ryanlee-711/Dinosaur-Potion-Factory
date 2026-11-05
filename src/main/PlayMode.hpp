#pragma once

#include "../starter/Mode.hpp"
#include "../starter/DrawLines.hpp"

#include "../rendering/Camera2D.hpp"
#include "../rendering/PixelCanvas.hpp"
#include "../rendering/Sprites.hpp"
#include "../rendering/Texture.hpp"
#include "../world/Player.hpp"
#include "../world/TileMap.hpp"
#include "../world/Entity.hpp"
#include "../inventory/Item.hpp"
#include "../inventory/ItemCursor.hpp"
#include "../world/Components.hpp"
#include "Rect.hpp"

#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <unordered_set>

struct Recipe;

// The main gameplay mode: a tilemap you can walk around on.
struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//update subroutines (make code cleaner)
	void do_world_interaction(float elapsed);
	bool place_machine(item_id_t const &id, glm::vec2 world_pos); // spawn the machine an item places; false if it isn't one
	bool interact_with(Entity *entity);  // run an entity's on_interact, false if it isn't interactable
	void interact_facing();              // E / Space: interact with what the dwarf faces, or close the open panel
	void consume_selected(); // F to drink/eat selected hotbar item
	void remove_objects(float elapsed);
	bool tile_in_reach(glm::ivec2 tile, float reach) const;

	// Helper methods
	template <typename EntityClass>
	void add_entity(std::vector<std::unique_ptr<Entity>> &list, glm::vec2 position);
	void spawn_item(item_id_t const &id, int count, glm::vec2 position);
	glm::ivec2 find_tilemap_coord(glm::vec2 screen_pos, PixelCanvas &view_canvas, glm::vec2 camera_topleft);
	Entity * entity_at(glm::ivec2 tile) const;
	void rebuild_coord_to_entities();

	//----- world -----
	Tilemap map;
	std::unique_ptr< Texture > tileset; // loaded from the path named in the map file
	Player player;
	std::vector<std::unique_ptr<Entity>> entities;

	// ----- entity lookup caching ------
	std::unordered_map<glm::ivec2, std::vector<Entity *>, TileHash> coord_to_entities;

	//----- recipe book -----
	bool book_open = false;
	size_t book_page = 0;                        // index of the recipe on the left page
	std::vector< Recipe const * > book_recipes;  // every potion recipe, one per page
	std::unordered_set< item_id_t > made_items;  // potions brewed at least once; their pages are unlocked
	Rect book_rect;                              // the whole open book on screen
	Rect book_prev_rect, book_next_rect;         // page-turn arrows
	void draw_book(Sprites &sprites, DrawLines &lines, glm::uvec2 drawable_size, float slot);
	void draw_book_page(Sprites &sprites, DrawLines &lines, Recipe const &recipe, Rect const &page, float slot, size_t page_number);

	// ----- quest UI -----
	struct Quest {
		std::string description = "";
		std::vector<ItemRecord> required_items { };
		int next_quest_index = -1;
	};
	// list of quests now
	std::vector<Quest> quests;
	std::vector<bool> quest_goals_cleared;
	int active_quest = 0;
	void draw_quest_ui(Sprites &sprites, DrawLines &lines, glm::uvec2 drawable_size);
	void clear_quest();

	//----- inventory -----
	static constexpr size_t HOTBAR_SLOTS = 10; // Number of slots in the hotbar
	static constexpr size_t BACKPACK_ROWS = 2; // Number of inventory rows
	Inventory inventory{HOTBAR_SLOTS * (1 + BACKPACK_ROWS)};
	bool inventory_open = false; // Is inventory showing
	Inventory *opened_inventory = nullptr;   // container panel currently open (nullptr = none)
	Entity *opened_entity = nullptr;         // the entity that owns opened_inventory
	int hotbar_selected = -1;   // -1 = unselected

	//----- inventory mouse UI -----
	ItemCursor cursor;  // items held on the mouse while a panel is open

	// Where one visible slot is on screen. Rebuilt every draw; also used for mouse hit-testing.
	struct SlotView {
		Inventory *inventory;
		size_t slot;
		Rect rect;  // drawable (physical) pixels
	};
	std::vector< SlotView > ui_slots;  // hotbar slots first, then the open container's

	bool show_arrow = false;
	Rect arrow_rect;

	bool ui_mouse_down = false;  // a left press landed on a slot and hasn't been released yet

	void layout_ui_slots(glm::uvec2 drawable_size, float slot_size);
	SlotView const *slot_at(glm::vec2 window_pos) const; // window (SDL event) coordinates
	bool mouse_over_ui() const;           // pointing at a slot or holding a slot press: the world ignores mouse buttons
	void show_slot_result(ItemCursor::Result result);
	void return_cursor_items();           // cursor -> player inventory (overflow dropped on the ground)
	void close_container();               // close the open panel; returns cursor items
	//----- input -----
	FrameInput input;

	//----- world ineraction -----
	IBreakable *currently_breaking = nullptr;    // the IBreakable that is the current target. nullptr for none
	float break_timer = 0.0f;
	struct MouseButton {
		int downs = 0;
		bool pressed = false;
		bool popped = false;
		bool latched = false;  // a press event arrived since the last update
	} left_click, right_click;
	glm::ivec2 hovered_coord = glm::ivec2{0};

	//short message from the last interaction ("Your bag is full"), shown for a few seconds:
	std::string message;
	float message_timer = 0.0f;

	//----- rendering -----
	Camera2D camera;
	Sprites batch;
	Sprites ui_batch;
	PixelCanvas canvas;
	float window_scale = 1.0f;   // related to retina schenanigans

	bool show_debug = false; // toggled with F3
};