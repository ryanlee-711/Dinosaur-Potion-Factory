#pragma once

#include "../starter/Mode.hpp"
#include "../starter/DrawLines.hpp"

#include "../rendering/Camera2D.hpp"
#include "../rendering/PixelCanvas.hpp"
#include "../rendering/Particles.hpp"
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
#include <unordered_map>
#include <unordered_set>

struct Recipe;
class Npc;

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
	Entity *interact_target() const;     // what E would act on right now. the E hint uses it too
	void consume_selected(); // F to drink/eat selected hotbar item
	void remove_objects(float elapsed);
	bool tile_in_reach(glm::ivec2 tile, float reach) const;

	// Helper methods
	template <typename EntityClass>
	void add_entity(std::vector<std::unique_ptr<Entity>> &list, glm::vec2 position, uint32_t uid=0);
	void add_machine(std::vector<std::unique_ptr<Entity>> &list, glm::vec2 position, item_id_t machine_type, uint32_t uid=0);
	void spawn_item(item_id_t const &id, int count, glm::vec2 position);
	glm::ivec2 find_tilemap_coord(glm::vec2 screen_pos, PixelCanvas &view_canvas, glm::vec2 camera_topleft);
	Entity * entity_at(glm::ivec2 tile) const;
	void rebuild_coord_to_entities();
	void rebuild_uid_to_entities();

	//----- world -----
	Tilemap map;
	std::unique_ptr< Texture > tileset; // loaded from the path named in the map file
	Player player;
	std::vector<std::unique_ptr<Entity>> entities;

	// ----- entity lookup caching ------
	std::unordered_map<glm::ivec2, std::vector<Entity *>, TileHash> coord_to_entities;
	std::unordered_map<uint32_t, Entity *> uid_to_entities;

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
	int dragon_dialogue_index = 0;   // which of the dragon's hints for the current quest it says next
	std::vector<Quest> quests;
	std::vector<bool> quest_goals_cleared;
	int active_quest = 0;
	void draw_quest_ui(Sprites &sprites, DrawLines &lines, glm::uvec2 drawable_size);
	void clear_quest();
	bool main_quest_ready() const;   // every goal of the current main quest is done: the dragon shows a "!"

	// ----- side quests: given by npcs, listed under the main quest while active -----
	struct SideQuest {
		std::string id;                       // saved in the save file
		std::string title;                    // shown in the quest list
		std::string giver;                    // dialogue id of who gives it
		std::vector<ItemRecord> required_items; // taken when you hand them over
		std::vector<ItemRecord> rewards;      // given when you hand them over
	};
	enum class SideQuestState : int { NotStarted = 0, Active = 1, Done = 2 };
	std::vector<SideQuest> side_quests;
	std::unordered_map<std::string, SideQuestState> side_quest_states; // a quest not in here hasn't started
	SideQuest const *find_side_quest(std::string const &id) const;
	SideQuestState side_quest_state(std::string const &id) const;
	bool side_quest_ready(SideQuest const &quest) const; // carrying everything it asks for
	bool npc_has_news(Npc const &npc) const;             // show a "!" over it: not met yet, a quest to give or turn in

	// ----- dialogue box (npcs and the dragon; lines from dist/data/dialogue.tsv) -----
	struct DialogueOption {
		enum class Action { Ask, QuestOffer, Accept, Decline, QuestWaiting, Give, Trade, DragonMore, Smith, Bye };
		std::string label;
		Action action;
		std::string key = "";  // Ask: the dialogue key of the answer
	};
	struct DialogueBox {
		bool open = false;
		Entity *with = nullptr;             // the npc being talked to
		std::string npc;                    // its dialogue id
		std::string speaker;                // name on the box
		std::vector<std::string> pages;     // one box at a time
		size_t page = 0;
		float shown_chars = 0.0f;           // typewriter: how much of the current page is visible
		std::vector<DialogueOption> options;
		std::vector<Rect> option_rects;     // where each option was drawn last frame (drawable pixels), for clicks
		Rect panel_rect;
	} dialogue;
	void open_dialogue(Entity *entity);                 // start talking to an npc / the dragon
	void say(std::vector<std::string> const &pages, std::vector<DialogueOption> options);
	std::vector<DialogueOption> dialogue_menu() const;  // the usual options for whoever we're talking to
	bool dialogue_options_visible() const;              // on the last page and done typing
	void dialogue_advance();                            // click / E: finish typing, or go to the next page
	void dialogue_choose(size_t index);
	void close_dialogue();
	void draw_dialogue(Sprites &sprites, DrawLines &lines, glm::uvec2 drawable_size, float slot);

	//----- inventory -----
	static constexpr size_t HOTBAR_SLOTS = 10; // Number of slots in the hotbar
	static constexpr size_t BACKPACK_ROWS = 2; // Number of inventory rows
	Inventory inventory{HOTBAR_SLOTS * (1 + BACKPACK_ROWS)};
	bool inventory_open = false; // Is inventory showing
	Inventory *opened_inventory = nullptr;   // container panel currently open (nullptr = none)
	Entity *opened_entity = nullptr;         // the entity that owns opened_inventory
	int hotbar_selected = -1;   // -1 = unselected

	//----- save and load -----
	static constexpr char const *initfile_path = "data/level1_init.save";
	static constexpr char const *savefile_path = "level1.save";
	void save();
	void load();

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
	int variety = 0;    // used when placing entities, representing the variation used
	Entity const * e_target = nullptr;   // used for drawing interaction target

	//short message from the last interaction ("Your bag is full"), shown for a few seconds:
	std::string message;
	float message_timer = 0.0f;
	float hint_time = 0.0f;              // drives the bob of the floating gather hints

	//----- rendering -----
	Camera2D camera;
	Sprites batch;
	Sprites ui_batch;
	PixelCanvas canvas;
	ParticleSystem particles;
	float window_scale = 1.0f;   // related to retina schenanigans

	bool show_debug = false; // toggled with F3
};