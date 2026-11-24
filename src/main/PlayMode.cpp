#include "PlayMode.hpp"

#include "GameConfig.hpp"

#include "../starter/DrawLines.hpp"
#include "../starter/Load.hpp"
#include "../starter/data_path.hpp"
#include "../starter/gl_errors.hpp"
#include "../starter/PathFont.hpp"

#include "../inventory/Effect.hpp"
#include "../inventory/Recipe.hpp"

#include "../world/Components.hpp"
#include "../world/World.hpp"
#include "../world/ItemEntity.hpp"
// Make macro for this?
#include "../world/entities/Pipe.hpp"
#include "../world/entities/Machine.hpp"
#include "../world/entities/AppleTree.hpp"
#include "../world/entities/Forageable.hpp"
#include "../world/entities/Boulder.hpp"
#include "../world/entities/OreVein.hpp"
#include "../world/entities/Tree.hpp"
#include "../world/entities/Creature.hpp"
#include "../world/entities/Dragon.hpp"
#include "../world/entities/Splitter.hpp"
#include "../world/entities/Golem.hpp"
#include "../world/entities/Npc.hpp"
#include "../world/entities/Bridge.hpp"
#include "../world/entities/Prop.hpp"
#include "../world/Dialogue.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include <random>
#include <sstream>
#include <fstream>

using GameConfig::TILE_SIZE;

static constexpr int PLAYER_FRAME_H = 32;

//How long an interaction message (e.g. "Your bag is full") stays on screen, in seconds.
static constexpr float MESSAGE_SECONDS = 2.5f;

//An open container panel closes by itself once the dwarf's feet are farther than this
static constexpr float CONTAINER_CLOSE_DISTANCE = float(GameConfig::TILE_SIZE);

//How long it takes to cause one damage by pickaxe to a machine
static constexpr float SECOND_PER_DAMAGE = 0.25f;

// Reach distances
static constexpr float PLACE_REACH = 6.0f;
static constexpr float TOOL_REACH = 3.0f;

// Dialogue box
static constexpr float DIALOGUE_CHARS_PER_SECOND = 60.0f; // typewriter speed

//The interactable entity whose hitbox overlaps 'reach' the most, or nullptr if none does
static Entity *find_interact_target(Rect const &reach, std::vector< std::unique_ptr< Entity > > const &entities) {
	Entity *best = nullptr;
	float best_area = 0.0f;
	for (auto const &entity : entities) {
		if (entity->removed) continue;
		if (dynamic_cast< IInteractable * >(entity.get()) == nullptr) continue;
		float area = reach.overlap_area(entity->hitbox());
		if (area > best_area) {
			best_area = area;
			best = entity.get();
		}
	}
	return best;
}

// Global textures

static Load< Texture > wavey_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/sprites/misc/waves.png"));
});

static Load< Texture > hints_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/test/hints.png")); //20x24 frames: 0 = E key bubble, 1 = empty bubble for a tool icon
});

static Load< Texture > quest_marker_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/test/quest_marker.png")); //7x13 "!" over npcs with something new to say
});

static Load< Texture > missing_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/sprites/items/missing_no.png"));
});

//----- inventory UI -----
//Draws an item icon filling 'rect' with its count in the bottom-right corner (screen pixels).
static void draw_item_stack(Sprites &batch, DrawLines &lines, ItemRecord const &record, Rect const &rect) {
	if (record.second <= 0) return;
	Item const *item = Item::get(record.first);
	if (item == nullptr) {
		batch.draw(*missing_texture, Rect{0.0f, 0.0f, 16.0f, 16.0f}, rect);
	} else {
		batch.draw(*item->texture, Rect{0.0f, 0.0f, 16.0f, 16.0f}, rect);
	}


	float text_h = rect.w * 0.375f; //count text height in screen pixels
	glm::vec3 anchor(rect.x + rect.w * 0.875f, rect.y + rect.h, 0.0f);
	std::string text = std::to_string(record.second);
	lines.draw_text(text, anchor + glm::vec3(2.0f, 2.0f, 0.0f), glm::vec3(text_h, 0.0f, 0.0f), glm::vec3(0.0f, -text_h, 0.0f), glm::u8vec4(0x00, 0x00, 0x00, 0xff));
	lines.draw_text(text, anchor, glm::vec3(text_h, 0.0f, 0.0f), glm::vec3(0.0f, -text_h, 0.0f), glm::u8vec4(0xff, 0xff, 0xff, 0xff));
}

//Width of 'text' as DrawLines::draw_text would draw it at height 1 (same glyph lookup), for centering text.
static float text_width(std::string const &text) {
	float width = 0.0f;
	size_t start = 0;
	while (start < text.size()) {
		size_t end = start;
		uint32_t glyph = -1U;
		while (end < text.size()) {
			end += 1;
			auto f = PathFont::font.glyph_map.find(text.substr(start, end - start));
			if (f == PathFont::font.glyph_map.end()) { end -= 1; break; }
			glyph = f->second;
		}
		if (glyph == -1U) { end = start + 1; width += 0.6f; } //unknown character: drawn as a 0.6-wide box
		else width += PathFont::font.glyph_widths[glyph];
		start = end;
	}
	return width;
}

//Background color of a slot, from its rule, so you can see what goes where.
static glm::u8vec4 slot_color(Inventory::SlotRule const &rule) {
	if (!rule.can_insert)                        return glm::u8vec4(0x8a, 0x6d, 0x1e, 0xc0); //output: gold
	if (rule.required_tags & ItemTag::Liquid)    return glm::u8vec4(0x24, 0x4c, 0x8c, 0xc0); //liquid: blue
	if (rule.required_tags & ItemTag::Crushable) return glm::u8vec4(0x6e, 0x68, 0x62, 0xc0); //crushable: stone
	return glm::u8vec4(0x22, 0x22, 0x22, 0x80);                                              //anything: dark
}

//A pixel-art arrow pointing right, filling 'r', built from 8 columns (shaft, then the head).
//The left 'progress' (0..1) of it is drawn in 'fill', the rest in 'empty'.
static void draw_arrow(Sprites &batch, Rect const &r, float progress, glm::u8vec4 empty, glm::u8vec4 fill, bool point_left = false) {
	static constexpr int heights[8] = {2, 2, 2, 2, 7, 5, 3, 1}; //column heights, in eighths of r.h
	float column = r.w / 8.0f;
	for (int c = 0; c < 8; ++c) {
		float h = r.h * float(heights[c]) / 8.0f;
		bool filled = (float(c) + 0.5f) / 8.0f <= progress;
		float x = point_left ? r.x + r.w - column * float(c + 1) : r.x + column * float(c); //mirror for left
		batch.fill_rect(Rect{x, r.y + (r.h - h) * 0.5f, column, h}, filled ? fill : empty);
	}
}

//----- recipe book colors -----
static glm::u8vec4 const BOOK_COVER(0x5a, 0x34, 0x1c, 0xff); //leather around the pages
static glm::u8vec4 const BOOK_PAPER(0xec, 0xdf, 0xbf, 0xff);
static glm::u8vec4 const BOOK_SPINE(0xc4, 0xb2, 0x8e, 0xff); //fold between the pages
static glm::u8vec4 const BOOK_INK(0x1c, 0x10, 0x08, 0xff);   //normal text
static glm::u8vec4 const BOOK_RED(0x6e, 0x12, 0x08, 0xff);   //effect lines
static glm::u8vec4 const BOOK_FADED(0x5a, 0x48, 0x30, 0xff); //page numbers, locked-page hint

//" I", " II", ... for an effect level
static std::string roman_level(int level) {
	static char const *numerals[] = {"", " I", " II", " III", " IV", " V"};
	if (level >= 0 && level <= 5) return numerals[level];
	return " " + std::to_string(level);
}

// ------ Quest system ------




std::mt19937 mt(0x64696e6f);   // "dino"

PlayMode::PlayMode() : map(data_path("maps/world.map")) {
	// initialize window scale, OS specific
	int window_w, window_h, drawable_w, drawable_h;
	SDL_GetWindowSize(Mode::window, &window_w, &window_h);
	SDL_GetWindowSizeInPixels(Mode::window, &drawable_w, &drawable_h);
	window_scale = float(drawable_w) / float(window_w);

	// initialize tileset
	tileset = std::make_unique< Texture >(data_path("assets/tileset.png"));
	//stand in the middle of the spawn tile, feet near its bottom:
	player.position = Tilemap::tile_center(glm::ivec2{136, 52}) + glm::vec2(0.0f, TILE_SIZE * 0.25f);
	player.uid = 15213;

	// recipe book: one page per recipe that makes a potion
	for (Recipe const *recipe : Recipe::all()) {
		if (recipe->items_out.empty()) continue;
		if (Item::tags_of(recipe->items_out[0].first) & ItemTag::Potion)
			book_recipes.push_back(recipe);
	}

	// initialize quest
	quests.emplace_back(
		Quest{"Collect drupes, crush them, and brew a drink.\nE: interact, F: drink, Shift: sprint, Tab: Open Inventory",
			std::vector<ItemRecord>{ItemRecord{"drupe", 6}, ItemRecord{"crushed_drupe", 3}, ItemRecord{"drupe_drink", 1}},
			1
		}
	);

	quests.emplace_back(
		Quest{"Use pickaxe to break & pick up furnace.",
			std::vector<ItemRecord>{ItemRecord{"pickaxe", 1}, ItemRecord{"furnace", 1}},
			2
		}
	);

	quests.emplace_back(
		Quest{"Give the dragon some copper ores to create pipes.",
			std::vector<ItemRecord>{ItemRecord{"copper_ore", 10}, ItemRecord{"pipe", 5}},
			3
		}
	);

	quests.emplace_back(
		Quest{"Brew stronger potion from dried lichen and drupe crush.",
			std::vector<ItemRecord>{ItemRecord{"dried_lichen", 5}, ItemRecord{"red_potion", 1}, ItemRecord{"strength_potion", 1}},
			4
		}
	);

	quests.emplace_back(
		Quest{"Semi-automate strength potion!?",
			std::vector<ItemRecord>{ItemRecord{"strength_potion", 8}},
			-1
		}
	);

	for (size_t i = 0; i < 10; ++i) quest_goals_cleared.emplace_back(false);

	// side quests (the npc that gives one names its id in the npc table, Npc.cpp)
	side_quests.emplace_back(SideQuest{
		"lumberjack_axe", "Bring Cole a Haste Potion", "lumberjack",
		std::vector<ItemRecord>{ItemRecord{"haste_potion", 1}},
		std::vector<ItemRecord>{ItemRecord{"axe", 1}},
	});
	side_quests.emplace_back(SideQuest{
		"ice_elf_lore", "Bring Nivra a Warmth Potion", "ice_elf",
		std::vector<ItemRecord>{ItemRecord{"warmth_potion", 1}},
		std::vector<ItemRecord>{}, //the reward is what she tells you ("after:" questions in dialogue.tsv)
	});

	side_quests.emplace_back(SideQuest{
		"bridge_fixing", "Get some planks to repair the bridge", "bridge",
		std::vector<ItemRecord>{ItemRecord{"planks", 10}},
		std::vector<ItemRecord>{},
	});

	load();

}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	//one-off key presses go here; held keys (movement) are read in update().
	if (evt.type == SDL_EVENT_KEY_DOWN && evt.key.key == SDLK_F3) {
		show_debug = !show_debug;
		return true;
	}
	//----- dialogue box: it takes the mouse and keys while it's open -----
	if (dialogue.open) {
		if (evt.type == SDL_EVENT_KEY_DOWN) {
			SDL_Keycode key = evt.key.key;
			if (key == SDLK_ESCAPE) {
				close_dialogue();
				return true;
			}
			if (key >= SDLK_1 && key <= SDLK_9) {
				//number keys pick an option
				if (dialogue_options_visible()) dialogue_choose(size_t(key - SDLK_1));
				return true;
			}
			if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
				dialogue_advance();
				return true;
			}
			if (key == SDLK_TAB || key == SDLK_J) close_dialogue(); //then open the inventory / book as usual below
		}
		if (evt.type == SDL_EVENT_MOUSE_BUTTON_DOWN && evt.button.button == SDL_BUTTON_LEFT) {
			glm::vec2 p = glm::vec2(evt.button.x, evt.button.y) * window_scale;
			if (dialogue_options_visible()) {
				for (size_t i = 0; i < dialogue.option_rects.size(); ++i) {
					if (dialogue.option_rects[i].contains(p)) {
						dialogue_choose(i);
						return true;
					}
				}
			}
			dialogue_advance(); //a click anywhere else goes on to the next page
			return true;
		}
		if (evt.type == SDL_EVENT_MOUSE_BUTTON_UP && evt.button.button == SDL_BUTTON_LEFT) return true;
	}

	if (evt.type == SDL_EVENT_KEY_DOWN && evt.key.key == SDLK_F) {
		//F: drink / eat the selected consumable
		if (!inventory_open && !dialogue.open) consume_selected();
		return true;
	}
	if (evt.type == SDL_EVENT_KEY_DOWN && evt.key.key == SDLK_J) {
		//J: open / close the recipe book
		if (book_open) {
			book_open = false;
		} else {
			close_container();
			book_open = true;
		}
		return true;
	}
	if (evt.type == SDL_EVENT_KEY_DOWN && evt.key.key == SDLK_TAB) {
		if (inventory_open) close_container();
		else {
			inventory_open = true;
			book_open = false;
		}
		return true;
	}
	if (evt.type == SDL_EVENT_KEY_DOWN && evt.key.key == SDLK_ESCAPE) {
		//close the open container (and put anything on the cursor back in the bag)
		book_open = false;
		close_container();
		return true;
	}

	//----- inventory mouse controls (see ItemCursor.hpp for the full scheme) -----
	// Clicks are handled here, as events, so even very fast clicks are never missed.
	if (evt.type == SDL_EVENT_MOUSE_BUTTON_DOWN && evt.button.button == SDL_BUTTON_LEFT) {
		if (book_open) {
			//recipe book: the arrows turn two pages at a time
			glm::vec2 p = glm::vec2(evt.button.x, evt.button.y) * window_scale;
			if (book_prev_rect.contains(p) && book_page >= 2) book_page -= 2;
			else if (book_next_rect.contains(p) && book_page + 2 < book_recipes.size()) book_page += 2;
			if (book_rect.contains(p)) return true;
		}

		if (SlotView const *hit = slot_at(glm::vec2(evt.button.x, evt.button.y)); hit != nullptr) {
			if (!inventory_open) {
				//no panel open: clicking the hotbar selects that slot (click again to unselect), like the number keys
				int slot = int(hit->slot);
				hotbar_selected = (hotbar_selected == slot) ? -1 : slot;
			} else {
				//a panel is open: move items with the cursor
				bool shift = (SDL_GetModState() & SDL_KMOD_SHIFT) != 0;
				show_slot_result(cursor.click(*hit->inventory, hit->slot, shift));
			}
			ui_mouse_down = true; //the world ignores this press until it's released
			return true;
		}
		left_click.latched = true; //for the world: remembered until the next update, so quick clicks aren't missed
		return false;
	}
	if (evt.type == SDL_EVENT_MOUSE_BUTTON_UP && evt.button.button == SDL_BUTTON_LEFT && ui_mouse_down) {
		ui_mouse_down = false;
		return true;
	}
	return false;
}

void PlayMode::update(float elapsed) {
	hint_time += elapsed;
	if (hint_time > 100.0f * 3.14159265f) hint_time -= 100.0f * 3.14159265f; //whole bob cycles, so no visible jump

	//read held movement keys (scancodes = physical key positions, so WASD works on any keyboard layout):
	bool const *keys = SDL_GetKeyboardState(nullptr);
	if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT])  input.direction.x -= 1.0f;
	if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) input.direction.x += 1.0f;
	if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP])    input.direction.y -= 1.0f; //y grows downward
	if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN])  input.direction.y += 1.0f;
	if (dialogue.open) input.direction = glm::vec2(0.0f); //stand still while talking
	//Shift + WASD sprints. Not while the inventory screen is open, where Shift is for shift-clicking.
	input.sprint = (keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT]) && !inventory_open;
	if (keys[SDL_SCANCODE_E] || keys[SDL_SCANCODE_SPACE]) {
		input.interact.pressed = true;
		input.interact.downs += 1;
	} else {
		input.interact.pressed = false;
		input.interact.downs = 0;
	}
	if (keys[SDL_SCANCODE_KP_ENTER] || keys[SDL_SCANCODE_X]) {
		// nope
		input.quicksave.pressed = true;
		input.quicksave.downs += 1;
	} else {
		input.quicksave.pressed = false;
		input.quicksave.downs = 0;
	}
	if (keys[SDL_SCANCODE_R]) {
		// nope
		input.flip.pressed = true;
		input.flip.downs += 1;
	} else {
		input.flip.pressed = false;
		input.flip.downs = 0;
	}
	{//K0...K9
		const SDL_Scancode scancodes[10] = {
			SDL_SCANCODE_0, SDL_SCANCODE_1, SDL_SCANCODE_2, SDL_SCANCODE_3, SDL_SCANCODE_4,
			SDL_SCANCODE_5, SDL_SCANCODE_6, SDL_SCANCODE_7, SDL_SCANCODE_8, SDL_SCANCODE_9,
		};
		for (size_t index = 0; index < 10; ++index) {
			if (keys[scancodes[index]]) {
				input.numkeys[index].pressed = true;
				input.numkeys[index].downs += 1;
			} else {
				input.numkeys[index].pressed = false;
				input.numkeys[index].downs = 0;
			}
		}
	}

	//everything entities can collide with this frame:
	std::vector<std::unique_ptr<Entity>> spawned;
	World world{map, entities, player, coord_to_entities, uid_to_entities, mt, &spawned, &made_items, &particles};

	// Update player
	player.update(input, elapsed, world);

	// Close panel if walked away
	if (opened_entity != nullptr
	 && player.hitbox().distance_to(opened_entity->hitbox()) > CONTAINER_CLOSE_DISTANCE) {
		close_container();
	}

	// Rebuild entity mapping (before world interaction)
	rebuild_coord_to_entities();
	rebuild_uid_to_entities();

	// Handle interact (only on the first frame E is down):
	do_world_interaction(elapsed);

	if (input.quicksave.downs == 1) {
		// Using this as the save button
		save();

	}

	// Handle flip
	if (input.flip.downs == 1) {
		variety += 1;
	}

	// numkeys select/unselect hotbar items (1..9, 0 = tenth slot).
	// Moving items between inventories is done with the mouse (see handle_event).
	for (size_t raw_index = 0; raw_index < 10 && !inventory_open && !dialogue.open; raw_index++) {
		int index = int((raw_index + 9) % 10);
		if (input.numkeys[raw_index].downs != 1) continue;
		if (hotbar_selected == index) hotbar_selected = -1;
		else hotbar_selected = index;
		break;
	}
	// change Player's held item based on selection (-1 = nothing selected: don't index the inventory with it)
	if (hotbar_selected >= 0) player.held_item = inventory[size_t(hotbar_selected)];
	else player.held_item = ItemRecord{"", 0};

	// Update entities
	for (auto &entity : entities) {
		if (entity->removed) continue;
		entity->update(input, elapsed, world);
	}

	// Add entities spawned during the updates
	for (auto &entity : spawned) {
		entities.push_back(std::move(entity));
	}
	spawned.clear();
	particles.update(elapsed);

	// collect ItemEntities on ground by player
	{
		for (auto &entity : world.entities) {
			if (auto *target = dynamic_cast< ItemEntity * >(entity.get()); target) {
				auto target_inventory = target->get_inventory();
				// If the item is close enough, then the flag for absorption will be set true;
				// Then, we consume it
				target->attract = inventory.space_for(target_inventory->get_slot_id(0)) > 0;
				if (!target->is_absorbable()) continue;
				item_id_t item_id = target_inventory->get_slot_id(0);
				int item_count = target_inventory->get_slot_count(0);
				if (item_id == "") continue;
				if (item_count == 0) continue;

				int remain = inventory.add(item_id, item_count);
				target_inventory->remove(item_id, item_count - remain);
				// if the item are all absorbed, then we remove it from the scene.
				if (target_inventory->count(item_id) <= 0) {
					target->removed = true;
				}
			}
		}
	}

	{ // before draw function
		e_target = interact_target();
	}

	// Remove "removed" objects from the scene; and add ItemEntity if a container is broken.
	remove_objects(elapsed);

	// Quest system: update quest data.
	if (active_quest != -1) {
		auto &quest = quests[active_quest];
		int req_idx = 0;
		for (auto &record : quest.required_items) {
			if (inventory.count(record.first) >= record.second) {
				quest_goals_cleared[req_idx] = true;
			}
			req_idx ++;
		}
	}


	// Dialogue box: type out the current page
	if (dialogue.open) {
		if (dialogue.with != nullptr && dialogue.with->removed) close_dialogue();
		else if (dialogue.page < dialogue.pages.size()) {
			float length = float(dialogue.pages[dialogue.page].size());
			dialogue.shown_chars = std::min(length, dialogue.shown_chars + elapsed * DIALOGUE_CHARS_PER_SECOND);
		}
	}

	// Count down the on-screen message
	if (message_timer > 0.0f) {
		message_timer -= elapsed;
		if (message_timer <= 0.0f) message.clear();
	}

	// clear input direction
	input.direction = glm::vec2{0.0f, 0.0f};
}

void PlayMode::do_world_interaction(float elapsed) {
	// subroutine for handling world interaction;
	// or, in more common terms, what happens when the mouse is being clicked on the screen.
	// if player is empty-handed, or holding a pickaxe, then it breaks stuff.
	// if player is holding something, then it places the item.

	float mx, my;
	SDL_MouseButtonFlags buttons = SDL_GetMouseState(&mx, &my);
	bool left_clicked = (buttons & SDL_BUTTON_LMASK) != 0 || left_click.latched;
	left_click.latched = false;
	if (left_clicked) {
		left_click.downs += 1;
		left_click.pressed = true;
	} else {
		left_click.downs = 0;
		left_click.pressed = false;
		left_click.popped = true;
	}

	// The inventory UI owns the mouse while it points at a slot or a press on a slot is still held.
	// While an inventory panel is open, the hotbar can't be used in the world: left click doesn't
	// place/use/break anything (the mouse is for moving items). Right click and E / Space can
	// still interact.
	bool over_ui = mouse_over_ui();
	bool panel_open = inventory_open || book_open || dialogue.open;
	if (over_ui || panel_open) break_timer = 0.0f;
	bool left_new = !over_ui && !panel_open && left_click.downs == 1;
	bool left_held = !over_ui && !panel_open && left_click.downs > 1;
	// bool right_new = !over_ui && right_click.downs == 1;

	item_id_t item_id = inventory.get_slot_id(hotbar_selected);
	uint32_t item_tags = Item::tags_of(item_id);
	bool holding_placeable = (item_tags & ItemTag::Placeable) != 0;
	bool holding_tool = (item_tags & ItemTag::Tool) != 0;
	glm::ivec2 coord = find_tilemap_coord(glm::vec2{mx, my}, canvas, camera.view_min(canvas.size));

	// overwrite coords when player used interact key instead (compare Stardew Valley again)
	if (input.interact.downs == 1) {
		coord = player.facing_tile();
	}
	// This variable is used by drawing for highlight cursor
	hovered_coord = coord;

	// E / Space: interact with whatever the dwarf is facing. Pressing it again with a panel open closes the panel.
	if (input.interact.downs == 1) {
		interact_facing();
	}

	// Left click with a machine selected: place it (within PLACE_REACH, on an empty tile)
	if (left_new && holding_placeable && tile_in_reach(coord, PLACE_REACH)) {
		glm::vec2 world_pos = glm::vec2(coord) * float(GameConfig::TILE_SIZE);
		if (entity_at(coord) == nullptr && place_machine(item_id, world_pos)) {
			inventory.extract_from_slot(size_t(hotbar_selected), 1);
		} else {
			message = "Can't place that there";
			message_timer = MESSAGE_SECONDS;
		}
	}

	if (left_new && item_id == "empty_bottle") {
		auto tileid = map.tile_at(coord, 2);
		if (7 <= tileid % 16 && tileid % 16 <= 10 && 9 <= tileid / 16 && tileid / 16 <= 11) {
			inventory.remove("empty_bottle", 1);
			spawn_item("water", 1, player.position);
		}
	}

	// Click with a tool on a resource (ore vein: pickaxe, tree: axe): one hit, whatever comes loose pops out toward player
	if (left_new && holding_tool && tile_in_reach(coord, TOOL_REACH)) {
		Entity *entity = entity_at(coord);
		if (auto *resource = dynamic_cast< IMineable * >(entity); resource != nullptr) {
			item_id_t tool = resource->required_tool();
			if (item_id != tool) {
				Item const *needed = Item::get(tool);
				message = (needed ? needed->name : tool) + " needed";
				message_timer = MESSAGE_SECONDS;
			} else {
				int power = 1 + player.effect_level("strength"); //strength makes every hit count more
				std::vector< ItemRecord > drops;
				if (!resource->mine(power, player.position, drops)) {
					message = resource->empty_message();
					message_timer = MESSAGE_SECONDS;
				}
				glm::vec2 from = entity->position + glm::vec2(float(GameConfig::TILE_SIZE));
				for (ItemRecord const &drop : drops) {
					spawn_item(drop.first, drop.second, from);
					glm::vec2 toward = player.position - from;
					if (glm::length(toward) > 0.001f) {
						static_cast< ItemEntity * >(entities.back().get())->velocity = glm::normalize(toward) * 45.0f;
					}
				}
			}
		}
	}

	// Hold left click with the pickaxe: break the machine / boulder under the mouse (within TOOL_REACH)
	// (only the pickaxe: the axe is for trees, so it can't wreck your factory)
	if (left_held && item_id == "pickaxe" && tile_in_reach(coord, TOOL_REACH)) {
		auto entity = entity_at(coord);
		auto *target = dynamic_cast< IBreakable * >(entity);
		//per-hit feedback (wobble + particles, if the target has any): see IBreakable::on_break_hit.
		//Only when the dwarf is strong enough to actually break it; otherwise it doesn't budge.
		auto break_hit = [&]() {
			int strength = std::max(1, player.effect_level("strength"));
			if (IBreakable::MAX_HOLD_DAMAGE * strength < target->get_hardness()) return; //too hard: no reaction
			target->on_break_hit();
			if (ParticlePreset const *chips = target->break_hit_particles()) {
				Rect hb = entity->hitbox();
				particles.burst(*chips, glm::vec2(hb.x + hb.w * 0.5f, hb.y + hb.h)); //from the bottom front
			}
		};
		if (target != nullptr && target == currently_breaking) {
			int ticks_before = static_cast<int>(break_timer / SECOND_PER_DAMAGE);
			break_timer += elapsed;
			if (static_cast<int>(break_timer / SECOND_PER_DAMAGE) > ticks_before) break_hit(); //another damage tick landed
			int damages = static_cast<int>(break_timer / SECOND_PER_DAMAGE);
			damages = std::min(damages, IBreakable::MAX_HOLD_DAMAGE);
			if (player.effect_level("strength") > 0) {
				damages *= player.effect_level("strength");
			}
			if (damages >= IBreakable::MAX_HOLD_DAMAGE && target->get_hardness() == 99 && target->get_drop_id() != "") {
				// these are non-dropping source blocks
				spawn_item(target->get_drop_id(), 1, entity->position);
				break_timer -= float(IBreakable::MAX_HOLD_DAMAGE) * SECOND_PER_DAMAGE;
			}
			if (damages >= target->get_hardness()) {
				Rect hb = entity->hitbox();
				particles.burst(Presets::RockDebris, (hb.min() + hb.max()) * 0.5f);
				entity->removed = true;
			}
		} else {
			// changed breaking target or target is just null
			break_timer = 0.0f;
			currently_breaking = target;
			if (target != nullptr) break_hit(); //first swing lands right away
		}
	} else if (!left_held) {
		break_timer = 0.0f;
	}

	left_click.popped = false;
	right_click.popped = false;
}

bool PlayMode::place_machine(item_id_t const &id, glm::vec2 world_pos) {
	auto check_tile_empty = [this, world_pos](glm::ivec2 lo, glm::ivec2 hi) -> bool {
		glm::ivec2 base = Tilemap::tile_at_point(world_pos);
		for (int y = lo.y; y <= hi.y; ++y) {
			for (int x = lo.x; x <= hi.x; ++x) {
				if (entity_at(base + glm::ivec2(x, y)) != nullptr) {
					return false;
				}
			}
		}
		return true;
	};

	if (id == "chest" && check_tile_empty(glm::ivec2{1, 0}, glm::ivec2{1, 0})) {
		add_machine(entities, world_pos, "chest");
	} else if (id == "crusher" && check_tile_empty(glm::ivec2{0, -1}, glm::ivec2{3, 0})) {
		add_machine(entities, world_pos, "crusher");
	} else if (id == "brewer" && check_tile_empty(glm::ivec2{0, -1}, glm::ivec2{3, 0})) {
		add_machine(entities, world_pos, "brewer");
	} else if (id == "pipe") {
		add_entity<Pipe>(entities, world_pos);
	} else if (id == "furnace" && check_tile_empty(glm::ivec2{0, -1}, glm::ivec2{3, 0})) {
		add_machine(entities, world_pos, "furnace");
	} else if (id == "splitter") {
		add_entity<Splitter>(entities, world_pos);
		if (auto ptr = dynamic_cast<Splitter *>(entities.back().get()); ptr) {
			ptr->rotation = static_cast<Splitter::Rotation>(variety % 4);
		}
	} else if (id == "joiner") {
		add_entity<Splitter>(entities, world_pos);
		if (auto ptr = dynamic_cast<Splitter *>(entities.back().get()); ptr) {
			ptr->rotation = static_cast<Splitter::Rotation>(variety % 4);
		}
	} else if (id == "crossover") {
		add_entity<Splitter>(entities, world_pos);
		if (auto ptr = dynamic_cast<Splitter *>(entities.back().get()); ptr) {
			ptr->rotation = static_cast<Splitter::Rotation>(variety % 4);
		}
	} else if (id == "fermenter" && check_tile_empty(glm::ivec2{0, -1}, glm::ivec2{3, 0})) {
		add_machine(entities, world_pos, "fermenter");
	} else if (id == "distiller" && check_tile_empty(glm::ivec2{0, -1}, glm::ivec2{3, 0})) {
		add_machine(entities, world_pos, "distiller");
	} else if (id == "fuelbox" && check_tile_empty(glm::ivec2{1, 0}, glm::ivec2{1, 0})) {
		add_machine(entities, world_pos, "fuelbox");
	} else if (id == "golem") {
		add_entity<Golem>(entities, world_pos);
	} else {
		return false; //not a machine
	}
	return true;
}

bool PlayMode::interact_with(Entity *entity) {
	if (entity == nullptr || entity->removed) return false;
	auto *target = dynamic_cast< IInteractable * >(entity);
	if (target == nullptr) return false;

	if (opened_inventory != nullptr && entity != opened_entity) {
		close_container(); //interacting with something else: close the old panel
	}

	//people (and the dragon) talk instead: the dialogue box has the options (trade, smith, ...)
	if (dynamic_cast< Npc * >(entity) != nullptr || dynamic_cast< Dragon * >(entity) != nullptr) {
		open_dialogue(entity);
		return true;
	}

	IInteractable::Context ctx{player, inventory};
	target->on_interact(ctx);
	if (ctx.open_container != nullptr) {
		opened_inventory = ctx.open_container;
		opened_entity = entity;
		inventory_open = true;
		book_open = false;
	}

	if (!ctx.message.empty()) {
		message = ctx.message;
		message_timer = MESSAGE_SECONDS;
	}
	return true;
}

Entity *PlayMode::interact_target() const {
	//what's in front of the dwarf: the interactable overlapping the reach box the most
	Entity *target = find_interact_target(player.interact_box(), entities);
	if (target != nullptr) return target;
	Entity *in_front = entity_at(player.facing_tile());
	if (in_front != nullptr && dynamic_cast< IInteractable * >(in_front) != nullptr) return in_front;
	return nullptr;
}

void PlayMode::interact_facing() {
	if (dialogue.open) {
		dialogue_advance();
		return;
	}
	Entity *target = interact_target();

	//a panel is open then E closes it
	if (inventory_open && (target == nullptr || target == opened_entity)) {
		close_container();
		return;
	}
	interact_with(target); //(closes the old panel first if it's a different entity)
}

void PlayMode::consume_selected() {
	item_id_t id = inventory.get_slot_id(hotbar_selected);
	Consumable const *consumable = Consumable::get(id);
	if (consumable == nullptr) return; //nothing selected, or not something you can drink

	for (EffectGrant const &grant : consumable->grants) {
		player.add_effect(grant.effect, grant.seconds, grant.level);
	}

	// pop empty bottle out of nowhere
	spawn_item("empty_bottle", 1, player.position);

	inventory.extract_from_slot(size_t(hotbar_selected), 1);
}

bool PlayMode::tile_in_reach(glm::ivec2 tile, float reach) const {
	return glm::length(glm::vec2(Tilemap::tile_at_point(player.position) - tile)) <= reach;
}

void PlayMode::remove_objects(float elapsed) {
	World world{map, entities, player, coord_to_entities, uid_to_entities, mt};
	// Remove entities that asked to be removed this frame.
	if (opened_entity != nullptr && opened_entity->removed) {
		//its panel is open: close it before the inventory it points at is deleted
		close_container();
	}

	// On destruction: for IItemHolder, we would create popped items
	{
		std::vector<std::pair<ItemRecord, glm::vec2>> buffered_item_records;

		// first pass: find the list of broken items.
		// we cannot modify it on the fly because emplacing while enumerating is
		// dangerous (undefined?) behavior
		for (auto &entity : world.entities) {
			if (!entity->removed) continue;
			if (auto *target = dynamic_cast< IInventoryHolder * >(entity.get()); target) {
				// generate a bunch of items if inventory non-empty
				Inventory *dropped = target->get_inventory();
				for (auto const &item_record: *dropped) {
					if (item_record.second <= 0) continue;
					buffered_item_records.emplace_back(item_record, entity.get()->position);
				}
			}
			if (auto *target = dynamic_cast<IBreakable *>(entity.get()); target) {
				item_id_t drop_id = target->get_drop_id();
				if (drop_id != "") {
					buffered_item_records.emplace_back(ItemRecord{drop_id, 1}, entity.get()->position);
				}
			}
		}

		// second pass: add the items as entities
		for (auto const &[item_record, pos] : buffered_item_records) {
			// Add one item entity somewhere
			entities.push_back(std::make_unique<ItemEntity>(item_record.first, item_record.second));
			entities.back()->position = pos;
			// adjust its velocity
			auto *target = dynamic_cast< ItemEntity * >(entities.back().get());
			if (!target) {
				throw std::runtime_error{"ItemEntity just initialized failed?!"};
			}
			target->velocity.x += 50.0f * (float(mt()) / float(mt.max()) - 0.5f);
			target->velocity.y += 50.0f * (float(mt()) / float(mt.max()) - 0.5f);
		}
	}

	std::erase_if(entities, [](std::unique_ptr< Entity > const &entity) { return entity->removed; });
}

// ********************************************************************* //
// ============================    DRAW    ============================= //
// ********************************************************************* //

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//--- world, drawn into the low-res canvas ---
	canvas.resize(drawable_size);

	//aim the camera at the dwarf's body rather than their feet:
	camera.follow(player.position - glm::vec2(0.0f, PLAYER_FRAME_H * 0.5f), map.size_px(), canvas.size);
	glm::vec2 view_min = camera.view_min(canvas.size);
	glm::vec2 view_max = camera.view_max(canvas.size);

	canvas.bind();
	glClearColor(0.05f, 0.04f, 0.06f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	batch.begin(camera.world_to_clip(canvas.size));

	map.draw(batch, *tileset, view_min, view_max);
	particles.draw(batch, true);

	{ //player + entities, sorted so whatever stands lower on screen (bigger position.y) draws on top
		std::vector< Entity const * > sorted;
		sorted.reserve(entities.size() + 1);
		sorted.emplace_back(&player);
		for (auto const &entity : entities) sorted.emplace_back(entity.get());
		std::stable_sort(sorted.begin(), sorted.end(), [](Entity const *a, Entity const *b) {
			return a->position.y < b->position.y;
		});
		for (Entity const *entity : sorted) {
			entity->draw(batch);
		}
	}
	particles.draw(batch, false);

	//--- hint (see IHintable), over the closest thing only: the tool icon over things within tool reach (ore veins,
	//    trees, big boulders), or the E key over what the dwarf is facing (forageables, small boulders). Faded while
	//    you're not holding the tool or can't do it yet (hint_locked, e.g. no strength). Nothing over depleted things.
	if (!inventory_open && !book_open && !dialogue.open) {
		float bob = std::round(std::sin(hint_time * 4.0f) * 1.5f);
		auto draw_hint = [&](glm::vec2 anchor, Texture const *icon, uint8_t alpha) {
			//anchor = where the bubble's tail points (tail tip is row 22 of the 24px frame)
			glm::vec2 top_left = glm::round(anchor) - glm::vec2(10.0f, 23.0f) + glm::vec2(0.0f, bob);
			glm::u8vec4 tint(0xff, 0xff, 0xff, alpha);
			if (icon == nullptr) {
				batch.draw(*hints_texture, Rect{0.0f, 0.0f, 20.0f, 24.0f}, top_left, tint); //E key
			} else {
				batch.draw(*hints_texture, Rect{20.0f, 0.0f, 20.0f, 24.0f}, top_left, tint); //empty bubble
				batch.draw(*icon, Rect{0.0f, 0.0f, 16.0f, 16.0f}, top_left + glm::vec2(2.0f, 2.0f), tint);
			}
		};
		constexpr uint8_t READY = 0xff, FADED = 0x90;

		//only ONE hint at a time, on the closest candidate, so crowded spots stay readable.
		item_id_t held = inventory.get_slot_id(hotbar_selected);
		std::vector< glm::ivec2 > tiles;
		Entity const *best = nullptr;
		float best_distance = 0.0f;
		for (auto const &entity : entities) {
			if (entity->removed) continue;
			auto const *hintable = dynamic_cast< IHintable const * >(entity.get());
			if (hintable == nullptr || !hintable->show_hint()) continue;

			if (hintable->hint_tool().empty()) {
				if (entity.get() != e_target) continue; //E key: only on what E would act on right now
			} else {
				//tool: same reach rule as clicking it (any tile it covers within TOOL_REACH)
				tiles.clear();
				entity->get_occupied(&tiles);
				bool in_reach = std::any_of(tiles.begin(), tiles.end(), [&](glm::ivec2 t) { return tile_in_reach(t, TOOL_REACH); });
				if (!in_reach) continue;
			}

			//distance from the dwarf's feet to the closest point of its hitbox
			Rect r = entity->hitbox();
			glm::vec2 closest = glm::clamp(player.position, r.min(), r.max());
			float distance = glm::length(closest - player.position);
			if (best == nullptr || distance < best_distance) {
				best = entity.get();
				best_distance = distance;
			}
		}

		if (best != nullptr) {
			auto const *hintable = dynamic_cast< IHintable const * >(best);
			bool locked = hintable->hint_locked(player);
			item_id_t tool = hintable->hint_tool();
			if (tool.empty()) {
				draw_hint(hintable->hint_anchor(), nullptr, locked ? FADED : READY);
			} else {
				Item const *tool_item = Item::get(tool);
				draw_hint(hintable->hint_anchor(), tool_item ? tool_item->texture.get() : &*missing_texture,
					(held == tool && !locked) ? READY : FADED);
			}
		}
	}

	//--- "!" over npcs with something new (not met yet, a quest to give or turn in) and over the dragon when
	//    the main quest is ready to turn in. Not over the one showing the E key, nor while talking.
	if (!dialogue.open) {
		float bob = std::round(std::sin(hint_time * 3.0f) * 1.0f);
		for (auto const &entity : entities) {
			if (entity->removed || entity.get() == e_target) continue;
			glm::vec2 anchor(0.0f);
			if (auto const *npc = dynamic_cast< Npc const * >(entity.get())) {
				if (!npc_has_news(*npc)) continue;
				anchor = npc->marker_anchor();
			} else if (dynamic_cast< Dragon const * >(entity.get()) != nullptr) {
				if (!main_quest_ready()) continue;
				Rect r = entity->hitbox();
				anchor = glm::vec2(r.x + r.w * 0.5f, r.y - 2.0f);
			} else {
				continue;
			}
			glm::vec2 top_left = glm::round(anchor) - glm::vec2(3.0f, 13.0f) + glm::vec2(0.0f, bob);
			batch.draw(*quest_marker_texture, Rect{0.0f, 0.0f, 7.0f, 13.0f}, top_left);
		}
	}

	if (show_debug) {
		//solid tiles in red:
		glm::ivec2 first = glm::max(Tilemap::tile_at_point(view_min), glm::ivec2(0));
		glm::ivec2 last = glm::min(Tilemap::tile_at_point(view_max), glm::ivec2(map.size) - 1);
		for (int ty = first.y; ty <= last.y; ++ty) {
			for (int tx = first.x; tx <= last.x; ++tx) {
				if (map.is_solid(glm::ivec2(tx, ty))) {
					batch.fill_rect(Tilemap::tile_rect(glm::ivec2(tx, ty)), glm::u8vec4(0xff, 0x30, 0x30, 0x50));
				}
			}
		}
		//entity hitboxes in orange (solid ones block the player):
		for (auto const &entity : entities) {
			batch.fill_rect(entity->hitbox(), glm::u8vec4(0xff, 0x90, 0x20, 0x70));
		}
		//creature roaming zones outlined in green:
		for (auto const &entity : entities) {
			auto const *creature = dynamic_cast< Creature const * >(entity.get());
			if (creature == nullptr) continue;
			Rect z = creature->zone_rect();
			glm::u8vec4 green(0x40, 0xff, 0x60, 0xc0);
			batch.fill_rect(Rect{z.x, z.y, z.w, 1.0f}, green);
			batch.fill_rect(Rect{z.x, z.y + z.h - 1.0f, z.w, 1.0f}, green);
			batch.fill_rect(Rect{z.x, z.y, 1.0f, z.h}, green);
			batch.fill_rect(Rect{z.x + z.w - 1.0f, z.y, 1.0f, z.h}, green);
		}
		//the dwarf's reach (what E interacts with) in blue, collision box in yellow:
		batch.fill_rect(player.interact_box(), glm::u8vec4(0x40, 0x80, 0xff, 0xa0));
		batch.fill_rect(player.hitbox(), glm::u8vec4(0xff, 0xee, 0x00, 0xb0));
	}

	{ // draw hints for pipes and machines
		// only if holding an item.
		item_id_t item_id = inventory.get_slot_id(hotbar_selected);
		uint32_t item_tags = Item::tags_of(item_id);
		bool holding_hintable = (item_tags & ItemTag::Placeable) != 0;

		if (holding_hintable) {
			for (auto &entity : entities) {
				if (auto machine = dynamic_cast<Machine *>(entity.get()); machine) {
					machine->draw_hint(batch, wavey_texture, 0.0f);
				} else if (auto pipe = dynamic_cast<Pipe *>(entity.get()); pipe) {
					pipe->draw_hint(batch);
				}
			}
		}
	}

	if (!mouse_over_ui()) { // hovered-tile highlight (hidden while the mouse is on the inventory UI)
		uint32_t tags = Item::tags_of(inventory.get_slot_id(hotbar_selected));
		bool placeable = (tags & ItemTag::Placeable) != 0;
		bool tool = (tags & ItemTag::Tool) != 0;
		if ((placeable || tool) && !inventory_open && !book_open && !mouse_over_ui()) {
			float reach = tool ? TOOL_REACH : PLACE_REACH;
			bool in_reach = tile_in_reach(hovered_coord, reach);
			glm::u8vec4 color = in_reach ? glm::u8vec4(0xee, 0xdd, 0x44, 0x28)  //yellow: in reach
			                             : glm::u8vec4(0xee, 0x44, 0x44, 0x28); //red: too far
			batch.fill_rect(Tilemap::tile_rect(hovered_coord), color);
			// for placeable machines, draw a ghost sprite over
			if (auto def = MachineDef::get(inventory.get_slot_id(hotbar_selected))) {
				Rect rect = def->texture_rect;
				rect.y += def->texture_rect.h * (variety % 4);
				batch.draw(*def->texture, rect, Tilemap::tile_topleft(hovered_coord) + def->draw_offset, glm::u8vec4{0xff, 0xff, 0xff, 0x40});
			}
		}
	}

	batch.end();

	//--- scale the canvas up to the window ---
	canvas.present(batch, drawable_size);

	//--- full-resolution overlay (UI goes here) ---
	if (show_debug) {
		glm::mat4 screen_to_clip = glm::ortho(0.0f, float(drawable_size.x), float(drawable_size.y), 0.0f);
		DrawLines lines(screen_to_clip);

		glm::ivec2 tile = Tilemap::tile_at_point(player.position);
		std::string text = "tile (" + std::to_string(tile.x) + ", " + std::to_string(tile.y) + ")"
			+ "  [" + std::to_string(map.tile_at(tile, 0)) + "]"
			+ "  zoom " + std::to_string(canvas.scale) + "x"
			+ "  canvas " + std::to_string(canvas.size.x) + "x" + std::to_string(canvas.size.y);

		float H = 12.0f * float(canvas.scale) * 0.5f; //text height in screen pixels
		glm::vec3 anchor(H * 0.5f, H * 1.5f, 0.0f);
		//dark shadow, then white text on top:
		lines.draw_text(text, anchor + glm::vec3(2.0f, 2.0f, 0.0f), glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, -H, 0.0f), glm::u8vec4(0x00, 0x00, 0x00, 0xff));
		lines.draw_text(text, anchor, glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, -H, 0.0f), glm::u8vec4(0xff, 0xff, 0xff, 0xff));
	}

	//--- inventory UI (screen pixels, laid out from the window size) ---
	{
		glm::mat4 screen_to_clip = glm::ortho(0.0f, float(drawable_size.x), float(drawable_size.y), 0.0f);
		float slot = 16.0f * float(canvas.scale);
		layout_ui_slots(drawable_size, slot);

		float mouse_x, mouse_y;
		SDL_GetMouseState(&mouse_x, &mouse_y);
		glm::vec2 mouse_window(mouse_x, mouse_y);
		SlotView const *hovered = slot_at(mouse_window);

		{
			DrawLines lines(screen_to_clip);
			ui_batch.begin(screen_to_clip);
			if (inventory_open && !ui_slots.empty()) {
				//dark backdrop behind all visible slots so the open inventory reads as one panel
				glm::vec2 lo = ui_slots.front().rect.min(), hi = ui_slots.front().rect.max();
				for (SlotView const &view : ui_slots) {
					lo = glm::min(lo, view.rect.min());
					hi = glm::max(hi, view.rect.max());
				}
				float pad = slot * 0.3f;
				ui_batch.fill_rect(Rect{lo.x - pad, lo.y - pad, hi.x - lo.x + 2.0f * pad, hi.y - lo.y + 2.0f * pad},
					glm::u8vec4(0x10, 0x10, 0x18, 0xb0));
			}
			for (SlotView const &view : ui_slots) {
				Rect const &r = view.rect;
				bool selected = (view.inventory == &inventory && int(view.slot) == hotbar_selected);
				bool is_hovered = (&view == hovered);

				//border (yellow = selected hotbar slot, light = hovered), then the slot itself
				float border = 0.05f * slot;
				glm::u8vec4 border_color(0x66, 0x66, 0x66, 0xff);
				if (is_hovered) border_color = glm::u8vec4(0xdd, 0xdd, 0xdd, 0xff);
				if (selected) border_color = glm::u8vec4(0xbb, 0xbb, 0x66, 0xff);
				ui_batch.fill_rect(Rect{r.x - border, r.y - border, r.w + 2.0f * border, r.h + 2.0f * border}, border_color);
				ui_batch.fill_rect(r, slot_color(view.inventory->rule(view.slot)));

				draw_item_stack(ui_batch, lines, view.inventory->slots[view.slot], r);

				//while holding items, tint the slots that won't accept them
				if (!cursor.empty() && !view.inventory->can_insert(view.slot, cursor.held.first)) {
					ui_batch.fill_rect(r, glm::u8vec4(0xaa, 0x22, 0x22, 0x60));
				}
			}

			if (show_arrow) {
				//arrow from a machine's inputs to its outputs; fills up while the machine works
				float progress = 0.0f;
				if (auto *machine = dynamic_cast< IProgress * >(opened_entity)) progress = machine->get_progress();
				draw_arrow(ui_batch, arrow_rect, progress, glm::u8vec4(0x55, 0x55, 0x55, 0xff), glm::u8vec4(0xff, 0xff, 0xff, 0xff));
			}

			if (!message.empty() && !ui_slots.empty()) {
				//interaction message, just above the topmost row
				Rect first = ui_slots.front().rect;
				for (SlotView const &view : ui_slots) {
					if (view.inventory == &inventory && view.rect.y < first.y) first = view.rect;
				}
				float text_h = slot * 0.4f;
				glm::vec3 anchor(first.x, first.y - slot * 0.35f, 0.0f);
				lines.draw_text(message, anchor + glm::vec3(2.0f, 2.0f, 0.0f), glm::vec3(text_h, 0.0f, 0.0f), glm::vec3(0.0f, -text_h, 0.0f), glm::u8vec4(0x00, 0x00, 0x00, 0xff));
				lines.draw_text(message, anchor, glm::vec3(text_h, 0.0f, 0.0f), glm::vec3(0.0f, -text_h, 0.0f), glm::u8vec4(0xff, 0xff, 0xff, 0xff));
			} else if (std::string sel_id = inventory.get_slot_id(hotbar_selected); !ui_slots.empty() && sel_id != "") {
				auto item = Item::get(sel_id);
				std::string text = "";
				if (item != nullptr) {
					text = item->name + (item->description != "" ? "  -  " + item->description : "");
				} else {
					text = sel_id + " (error item)";
				}

				Rect const &first = ui_slots.front().rect;
				float text_h = slot * 0.4f;
				glm::vec3 anchor(first.x, first.y - slot * 0.35f, 0.0f);
				lines.draw_text(text, anchor + glm::vec3(2.0f, 2.0f, 0.0f), glm::vec3(text_h, 0.0f, 0.0f), glm::vec3(0.0f, -text_h, 0.0f), glm::u8vec4(0x00, 0x00, 0x00, 0xff));
				lines.draw_text(text, anchor, glm::vec3(text_h, 0.0f, 0.0f), glm::vec3(0.0f, -text_h, 0.0f), glm::u8vec4(0xff, 0xff, 0xff, 0xff));
			}
			ui_batch.end();
		} //(lines draw here, on top of the slots)

		//name of the hovered item, centered above its slot
		if (hovered != nullptr && cursor.empty() && !hovered->inventory->slot_empty(hovered->slot)) {
			if (Item const *item = Item::get(hovered->inventory->slots[hovered->slot].first)) {
				Rect const &r = hovered->rect;
				float text_h = slot * 0.4f;
				float text_w = text_width(item->name) * text_h;
				float pad = slot * 0.12f;
				//background box, just above the slot
				Rect box{std::floor(r.x + (r.w - text_w) * 0.5f - pad), std::floor(r.y - slot * 0.15f - text_h - 2.0f * pad), text_w + 2.0f * pad, text_h + 2.0f * pad};
				DrawLines lines(screen_to_clip);
				ui_batch.begin(screen_to_clip);
				ui_batch.fill_rect(box, glm::u8vec4(0x18, 0x10, 0x28, 0xe8));
				ui_batch.end();
				glm::vec3 anchor(box.x + pad, box.y + pad + text_h, 0.0f); //text baseline
				lines.draw_text(item->name, anchor, glm::vec3(text_h, 0.0f, 0.0f), glm::vec3(0.0f, -text_h, 0.0f), glm::u8vec4(0xff, 0xff, 0xff, 0xff));
			}
		}

		//the stack held on the cursor, drawn last so it sits above everything
		if (!cursor.empty()) {
			glm::vec2 mouse = mouse_window * window_scale;
			Rect r{std::floor(mouse.x - slot * 0.5f), std::floor(mouse.y - slot * 0.5f), slot, slot};
			DrawLines lines(screen_to_clip);
			ui_batch.begin(screen_to_clip);
			draw_item_stack(ui_batch, lines, cursor.held, r);
			ui_batch.end();
		}
	}

	//---- quest system (main quest, then active side quests) ----
	{
		glm::mat4 screen_to_clip = glm::ortho(0.0f, float(drawable_size.x), float(drawable_size.y), 0.0f);
		DrawLines lines(screen_to_clip);
		draw_quest_ui(ui_batch, lines, drawable_size);
	}

	//---- dialogue box ----
	if (dialogue.open) {
		glm::mat4 screen_to_clip = glm::ortho(0.0f, float(drawable_size.x), float(drawable_size.y), 0.0f);
		float slot = 16.0f * float(canvas.scale);
		DrawLines lines(screen_to_clip);
		ui_batch.begin(screen_to_clip);
		draw_dialogue(ui_batch, lines, drawable_size, slot);
		ui_batch.end();
	}


	//--- recipe book ---
	if (book_open) {
		glm::mat4 screen_to_clip = glm::ortho(0.0f, float(drawable_size.x), float(drawable_size.y), 0.0f);
		float slot = 16.0f * float(canvas.scale);
		DrawLines lines(screen_to_clip);
		ui_batch.begin(screen_to_clip);
		draw_book(ui_batch, lines, drawable_size, slot);
		ui_batch.end();
	}

	//--- active effects, top right (the quest list is top left) ---
	if (!player.active_effects().empty()) {
		glm::mat4 screen_to_clip = glm::ortho(0.0f, float(drawable_size.x), float(drawable_size.y), 0.0f);
		DrawLines lines(screen_to_clip);
		float text_h = 8.0f * float(canvas.scale);
		float y = text_h * 1.5f;
		for (auto const &[id, active] : player.active_effects()) {
			Effect const *effect = Effect::get(id);
			std::string text = (effect ? effect->name : id) + roman_level(active.level)
				+ ":  " + std::to_string(int(std::ceil(active.time_left))) + "s";
			//right-aligned: start one text width (plus a margin) in from the right edge
			float x = std::floor(float(drawable_size.x) - text_h * 0.5f - text_width(text) * text_h);
			glm::vec3 anchor(x, y, 0.0f);
			lines.draw_text(text, anchor + glm::vec3(2.0f, 2.0f, 0.0f), glm::vec3(text_h, 0.0f, 0.0f), glm::vec3(0.0f, -text_h, 0.0f), glm::u8vec4(0x00, 0x00, 0x00, 0xff));
			lines.draw_text(text, anchor, glm::vec3(text_h, 0.0f, 0.0f), glm::vec3(0.0f, -text_h, 0.0f), glm::u8vec4(0xff, 0xf0, 0xa0, 0xff));
			y += text_h * 1.4f;
		}
	}

	GL_ERRORS();
}

// =================== Recipe book ================= //

void PlayMode::draw_book(Sprites &sprites, DrawLines &lines, glm::uvec2 drawable_size, float slot) {
	glm::vec2 screen = glm::vec2(drawable_size);

	//two pages side by side, centered in the space above the hotbar
	float page_w = std::floor(slot * 9.0f);
	float page_h = std::floor(slot * 8.0f);
	float gutter = std::floor(slot * 0.25f); //fold between the pages
	float border = std::floor(slot * 0.3f);  //cover showing around the pages
	glm::vec2 top_left = glm::floor(glm::vec2(
		(screen.x - 2.0f * page_w - gutter) * 0.5f,
		(screen.y - slot * 2.0f - page_h) * 0.5f
	));
	Rect left{top_left.x, top_left.y, page_w, page_h};
	Rect right{top_left.x + page_w + gutter, top_left.y, page_w, page_h};
	book_rect = Rect{left.x - border, left.y - border, 2.0f * page_w + gutter + 2.0f * border, page_h + 2.0f * border};

	sprites.fill_rect(book_rect, BOOK_COVER);
	sprites.fill_rect(left, BOOK_PAPER);
	sprites.fill_rect(right, BOOK_PAPER);
	sprites.fill_rect(Rect{left.x + page_w, left.y, gutter, page_h}, BOOK_SPINE);

	book_prev_rect = Rect{};
	book_next_rect = Rect{};
	if (book_recipes.empty()) return;

	//left page shows an even index; the right page the one after it (if any)
	book_page = std::min(book_page, (book_recipes.size() - 1) / 2 * 2);
	draw_book_page(sprites, lines, *book_recipes[book_page], left, slot, book_page + 1);
	if (book_page + 1 < book_recipes.size()) {
		draw_book_page(sprites, lines, *book_recipes[book_page + 1], right, slot, book_page + 2);
	}

	//page-turn arrows in the bottom outer corners, only when there's a page that way
	float a = std::floor(slot * 0.75f);
	float arrow_y = left.y + page_h - a - std::floor(slot * 0.25f);
	if (book_page >= 2) {
		book_prev_rect = Rect{left.x + std::floor(slot * 0.3f), arrow_y, a, a};
		draw_arrow(sprites, book_prev_rect, 0.0f, BOOK_INK, BOOK_INK, true);
	}
	if (book_page + 2 < book_recipes.size()) {
		book_next_rect = Rect{right.x + page_w - a - std::floor(slot * 0.3f), arrow_y, a, a};
		draw_arrow(sprites, book_next_rect, 0.0f, BOOK_INK, BOOK_INK);
	}
}


void PlayMode::draw_book_page(Sprites &sprites, DrawLines &lines, Recipe const &recipe, Rect const &page, float slot, size_t page_number) {
	//draws 'str' with the left end of its baseline at (x, y), 'h' pixels tall
	auto text = [&](std::string const &str, float x, float y, float h, glm::u8vec4 color) {
		lines.draw_text(str, glm::vec3(x, y, 0.0f), glm::vec3(h, 0.0f, 0.0f), glm::vec3(0.0f, -h, 0.0f), color);
	};
	Rect const sprite_src{0.0f, 0.0f, 16.0f, 16.0f};

	float pad = std::floor(slot * 0.5f);
	float x = page.x + pad;
	float body_h = std::floor(slot * 0.35f);
	float line = std::floor(body_h * 1.6f);

	item_id_t potion = recipe.items_out.empty() ? "" : recipe.items_out[0].first;
	Item const *item = Item::get(potion);

	//--- potion sprite, top right corner ---
	float icon = std::floor(slot * 1.5f);
	Rect icon_rect{page.x + page.w - pad - icon, page.y + std::floor(pad * 0.6f), icon, icon};

	//--- locked: not brewed yet. Show "????" and a dark silhouette of the potion, nothing else ---
	if (made_items.count(potion) == 0) {
		if (item) sprites.draw(*item->texture, sprite_src, icon_rect, glm::u8vec4(0x00, 0x00, 0x00, 0xff));
		float title_h = std::floor(slot * 0.55f);
		text("????", x, page.y + pad + title_h, title_h, BOOK_INK);
		text("Brew this potion once", x, icon_rect.y + icon + line, body_h, BOOK_FADED);
		text("to learn its recipe.", x, icon_rect.y + icon + 2.0f * line, body_h, BOOK_FADED);
	} else {
		if (item) sprites.draw(*item->texture, sprite_src, icon_rect);

		//--- name, shrunk if needed so it never runs into the sprite ---
		std::string name = item ? item->name : potion;
		float title_space = icon_rect.x - x - slot * 0.2f;
		float title_h = std::min(slot * 0.55f, title_space / std::max(text_width(name), 0.01f));
		text(name, x, page.y + pad + title_h, title_h, BOOK_INK);

		//--- effects (from the consumables table) ---
		float y = icon_rect.y + icon + line;
		if (Consumable const *drink = Consumable::get(potion)) {
			for (EffectGrant const &grant : drink->grants) {
				Effect const *effect = Effect::get(grant.effect);
				std::string str = (effect ? effect->name : grant.effect) + roman_level(grant.level)
					+ " for " + std::to_string(int(grant.seconds)) + "s";
				text(str, x, y, body_h, BOOK_RED);
				y += line;
			}
		} else {
			text("No effect: brewing base", x, y, body_h, BOOK_RED);
			y += line;
		}

		//--- ingredients: small sprite as the bullet, then "3 x Apple Powder" ---
		y += std::floor(line * 0.5f);
		float header_h = std::floor(body_h * 1.15f);
		text("Ingredients", x, y, header_h, BOOK_INK);
		float underline = std::max(1.0f, std::floor(slot / 16.0f));
		sprites.fill_rect(Rect{x, y + std::floor(body_h * 0.25f), text_width("Ingredients") * header_h, underline}, BOOK_INK);
		y += line;

		float bullet = std::floor(body_h * 1.4f);
		for (ItemRecord const &in : recipe.items_in) {
			Item const *ingredient = Item::get(in.first);
			if (ingredient) {
				//centered on the text's middle (the text sits between y - body_h and y)
				Rect bullet_rect{x, std::floor(y - body_h * 0.5f - bullet * 0.5f), bullet, bullet};
				sprites.draw(*ingredient->texture, sprite_src, bullet_rect);
			}
			std::string str = std::to_string(in.second) + " x " + (ingredient ? ingredient->name : in.first);
			text(str, x + bullet + std::floor(slot * 0.15f), y, body_h, BOOK_INK);
			y += std::floor(bullet * 1.2f);
		}

		//--- machine (and how long it takes) ---
		y += std::floor(line * 0.5f);
		Item const *machine = Item::get(recipe.machine_id);
		std::string machine_name = machine ? machine->name : recipe.machine_id;
		text("Made in: " + machine_name + " (" + std::to_string(int(recipe.time_cost)) + "s)", x, y, body_h, BOOK_INK);
	}

	//--- page number, bottom center ---
	std::string number = std::to_string(page_number);
	float number_h = std::floor(body_h * 0.9f);
	text(number, std::floor(page.x + (page.w - text_width(number) * number_h) * 0.5f), page.y + page.h - std::floor(pad * 0.6f), number_h, BOOK_FADED);
}

// =================== Quest UI ====================== //


void PlayMode::draw_quest_ui(Sprites &sprites, DrawLines &lines, glm::uvec2 drawable_size) {
	float text_h = 8.0f * float(canvas.scale);
	float x = std::floor(text_h * 0.5f);
	float y = text_h * 1.5f; //top left (effects are drawn top right)

	//one line of text with a drop shadow, then move down a line
	auto line = [&](std::string const &text, glm::u8vec4 color) {
		glm::vec3 anchor(x, y, 0.0f);
		lines.draw_text(text, anchor + glm::vec3(2.0f, 2.0f, 0.0f), glm::vec3(text_h, 0.0f, 0.0f), glm::vec3(0.0f, -text_h, 0.0f), glm::u8vec4(0x00, 0x00, 0x00, 0xff));
		lines.draw_text(text, anchor, glm::vec3(text_h, 0.0f, 0.0f), glm::vec3(0.0f, -text_h, 0.0f), color);
		y += text_h * 1.4f;
	};
	glm::u8vec4 const DONE(0xbf, 0xff, 0x90, 0xff), TODO(0xf0, 0xab, 0x90, 0xff);
	//"[ X ] Haste Potion  1/1"
	auto goal = [&](ItemRecord const &record, bool done) {
		Item const *item = Item::get(record.first);
		std::string name = item ? item->name : record.first;
		int have = done ? record.second : std::min(inventory.count(record.first), record.second);
		line(std::string(done ? "[ X ] " : "[   ] ") + name + "  " + std::to_string(have) + "/" + std::to_string(record.second), done ? DONE : TODO);
	};

	//--- main quest ---
	if (active_quest >= 0 && size_t(active_quest) < quests.size()) {
		std::stringstream ss("[Goals]  " + quests[active_quest].description);
		std::string text;
		while (std::getline(ss, text)) line(text, glm::u8vec4(0xff, 0xff, 0xf0, 0xff));

		int req_idx = -1;
		for (auto &record : quests[active_quest].required_items) {
			req_idx++;
			if (Item::get(record.first) == nullptr) continue;
			goal(record, quest_goals_cleared[req_idx]);
		}
		if (main_quest_ready()) line("Talk to the dragon", DONE);
	}

	//--- side quests in progress ---
	for (SideQuest const &quest : side_quests) {
		if (side_quest_state(quest.id) != SideQuestState::Active) continue;
		y += text_h * 0.4f;
		line("[Side]  " + quest.title, glm::u8vec4(0xa8, 0xd8, 0xff, 0xff));
		for (ItemRecord const &record : quest.required_items) goal(record, inventory.count(record.first) >= record.second);
		if (side_quest_ready(quest)) line("Talk to " + Dialogue::name(quest.giver), DONE);
	}
}

void PlayMode::clear_quest() {
	// this function tries to clear the quest if it is clearable.
	if (active_quest < 0 || size_t(active_quest) >= quests.size()) return; //all main quests done
	int req_idx = -1;
	for (auto &record : quests[active_quest].required_items) {
		req_idx++;
		auto item = Item::get(record.first);
		if (item == nullptr) continue;

		if (!quest_goals_cleared[req_idx]) return;
	}
	// should be cleared. proceed to next task
	for (size_t i = 0; i < quest_goals_cleared.size(); ++i)
		quest_goals_cleared[i] = false;

	active_quest = quests[active_quest].next_quest_index;
}

bool PlayMode::main_quest_ready() const {
	if (active_quest < 0 || size_t(active_quest) >= quests.size()) return false;
	int req_idx = -1;
	for (auto const &record : quests[active_quest].required_items) {
		req_idx++;
		if (Item::get(record.first) == nullptr) continue;
		if (!quest_goals_cleared[req_idx]) return false;
	}
	return true;
}

// =================== Side quests ====================== //

PlayMode::SideQuest const *PlayMode::find_side_quest(std::string const &id) const {
	for (SideQuest const &quest : side_quests) {
		if (quest.id == id) return &quest;
	}
	return nullptr;
}

PlayMode::SideQuestState PlayMode::side_quest_state(std::string const &id) const {
	auto f = side_quest_states.find(id);
	return f == side_quest_states.end() ? SideQuestState::NotStarted : f->second;
}

bool PlayMode::side_quest_ready(SideQuest const &quest) const {
	for (ItemRecord const &record : quest.required_items) {
		if (inventory.count(record.first) < record.second) return false;
	}
	return true;
}

bool PlayMode::npc_has_news(Npc const &npc) const {
	if (!npc.met) return true;
	SideQuest const *quest = find_side_quest(npc.definition().side_quest);
	if (quest == nullptr) return false;
	SideQuestState state = side_quest_state(quest->id);
	if (state == SideQuestState::NotStarted) return true;
	return state == SideQuestState::Active && side_quest_ready(*quest);
}

// =================== Dialogue box ====================== //

void PlayMode::open_dialogue(Entity *entity) {
	close_container();
	book_open = false;
	dialogue = DialogueBox{};
	dialogue.open = true;
	dialogue.with = entity;

	if (auto *npc = dynamic_cast< Npc * >(entity)) {
		IInteractable::Context ctx{player, inventory};
		npc->on_interact(ctx); //turn toward the dwarf
		dialogue.npc = npc->definition().dialogue;
		dialogue.speaker = Dialogue::name(dialogue.npc);
		std::vector<std::string> pages;
		if (!npc->met) pages = Dialogue::lines(dialogue.npc, "greet");
		if (pages.empty()) pages.push_back(Dialogue::random_line(dialogue.npc, "hello"));
		npc->met = true;
		say(pages, dialogue_menu());
	} else {
		//the dragon: turning in the main quest happens just by talking to it
		dialogue.npc = "dragon";
		dialogue.speaker = Dialogue::name("dragon");
		dragon_dialogue_index = 0;
		clear_quest();
		auto const &hints = Dialogue::lines("dragon", "quest" + std::to_string(active_quest));
		say(std::vector<std::string>{hints.empty() ? std::string("...") : hints[0]}, dialogue_menu());
	}
}

void PlayMode::say(std::vector<std::string> const &pages, std::vector<DialogueOption> options) {
	dialogue.pages.clear();
	for (std::string const &page : pages) {
		if (!page.empty()) dialogue.pages.push_back(page);
	}
	if (dialogue.pages.empty()) dialogue.pages.push_back("...");
	dialogue.page = 0;
	dialogue.shown_chars = 0.0f;
	dialogue.options = std::move(options);
	dialogue.option_rects.clear();
}

std::vector<PlayMode::DialogueOption> PlayMode::dialogue_menu() const {
	using Action = DialogueOption::Action;
	std::vector<DialogueOption> options;

	if (dynamic_cast< Dragon * >(dialogue.with) != nullptr) {
		options.push_back(DialogueOption{"Tell me more", Action::DragonMore});
		options.push_back(DialogueOption{"Smith something", Action::Smith});
		options.push_back(DialogueOption{"Bye", Action::Bye});
		return options;
	}

	auto *npc = dynamic_cast< Npc * >(dialogue.with);
	if (npc == nullptr) return options;
	NpcDef const &def = npc->definition();

	//its side quest (no quest counts as done)
	SideQuest const *quest = find_side_quest(def.side_quest);
	SideQuestState state = quest ? side_quest_state(quest->id) : SideQuestState::Done;

	//questions, in the order they're in dialogue.tsv ("before:" ones until the side quest is done,
	//"after:" ones once it's done)
	for (auto const &[question, key] : Dialogue::questions(def.dialogue)) {
		bool quest_done = (state == SideQuestState::Done);
		if (key.rfind("before:", 0) == 0 && quest_done) continue;
		if (key.rfind("after:", 0) == 0 && !quest_done) continue;
		options.push_back(DialogueOption{question, Action::Ask, key});
	}

	if (quest) {
		if (state == SideQuestState::NotStarted) {
			options.push_back(DialogueOption{"Need a hand?", Action::QuestOffer});
		} else if (state == SideQuestState::Active) {
			//"Give Haste Potion" once you carry everything, otherwise ask about it
			std::string items;
			for (ItemRecord const &record : quest->required_items) {
				Item const *item = Item::get(record.first);
				if (!items.empty()) items += ", ";
				items += (record.second > 1 ? std::to_string(record.second) + " " : "") + (item ? item->name : record.first);
			}
			if (side_quest_ready(*quest)) options.push_back(DialogueOption{"Give " + items, Action::Give});
			else options.push_back(DialogueOption{"About the " + items + "...", Action::QuestWaiting});
		}
	}

	//trade
	if (def.trades() && (!def.trade_after_quest || state == SideQuestState::Done)) {
		options.push_back(DialogueOption{"Trade", Action::Trade});
	}

	options.push_back(DialogueOption{"Bye", Action::Bye});
	return options;
}

bool PlayMode::dialogue_options_visible() const {
	if (!dialogue.open || dialogue.pages.empty()) return false;
	if (dialogue.page + 1 < dialogue.pages.size()) return false;
	return dialogue.shown_chars >= float(dialogue.pages[dialogue.page].size());
}

void PlayMode::dialogue_advance() {
	if (!dialogue.open || dialogue.pages.empty()) return;
	float length = float(dialogue.pages[dialogue.page].size());
	if (dialogue.shown_chars < length) {
		dialogue.shown_chars = length; //still typing: show the whole page
	} else if (dialogue.page + 1 < dialogue.pages.size()) {
		dialogue.page += 1;
		dialogue.shown_chars = 0.0f;
	}
	//(last page: wait for an option)
}

void PlayMode::dialogue_choose(size_t index) {
	if (index >= dialogue.options.size()) return;
	DialogueOption option = dialogue.options[index]; //copy: say() replaces the options
	using Action = DialogueOption::Action;

	auto *npc = dynamic_cast< Npc * >(dialogue.with);
	SideQuest const *quest = npc ? find_side_quest(npc->definition().side_quest) : nullptr;
	std::string const &who = dialogue.npc;

	switch (option.action) {
	case Action::Ask:
		say(Dialogue::lines(who, option.key), dialogue_menu());
		break;
	case Action::QuestOffer:
		say(Dialogue::lines(who, "quest_offer"), std::vector<DialogueOption>{
			DialogueOption{"Sure, I'll do it", Action::Accept},
			DialogueOption{"Not right now", Action::Decline},
		});
		break;
	case Action::Accept:
		if (quest) side_quest_states[quest->id] = SideQuestState::Active;
		say(Dialogue::lines(who, "quest_accept"), dialogue_menu());
		break;
	case Action::Decline:
		say(Dialogue::lines(who, "quest_decline"), dialogue_menu());
		break;
	case Action::QuestWaiting:
		say(Dialogue::lines(who, "quest_waiting"), dialogue_menu());
		break;
	case Action::Give:
		if (quest && side_quest_ready(*quest)) {
			for (ItemRecord const &record : quest->required_items) inventory.remove(record.first, record.second);
			std::string got;
			for (ItemRecord const &record : quest->rewards) {
				int left = inventory.add(record.first, record.second);
				if (left > 0) spawn_item(record.first, left, player.position); //bag full: at the dwarf's feet
				Item const *item = Item::get(record.first);
				got += (got.empty() ? "" : ", ") + (item ? item->name : record.first);
			}
			side_quest_states[quest->id] = SideQuestState::Done;
			if (auto *bridge = dynamic_cast<Bridge *>(npc)) {
				bridge->repair();
			}
			if (!got.empty()) {
				message = "Got: " + got;
				message_timer = MESSAGE_SECONDS;
			}
			say(Dialogue::lines(who, "quest_done"), dialogue_menu());
		} else {
			say(Dialogue::lines(who, "quest_waiting"), dialogue_menu());
		}
		break;
	case Action::Trade:
		if (npc) {
			close_dialogue();
			opened_inventory = &npc->trade_inventory();
			opened_entity = npc;
			inventory_open = true;
			book_open = false;
		}
		break;
	case Action::DragonMore: {
		dragon_dialogue_index += 1;
		auto const &hints = Dialogue::lines("dragon", "quest" + std::to_string(active_quest));
		std::string text = hints.empty() ? std::string("...") : hints[size_t(dragon_dialogue_index) % hints.size()];
		say(std::vector<std::string>{text}, dialogue_menu());
		break;
	}
	case Action::Smith: {
		Entity *dragon = dialogue.with;
		close_dialogue();
		if (auto *target = dynamic_cast< IInteractable * >(dragon)) {
			IInteractable::Context ctx{player, inventory};
			target->on_interact(ctx); //opens its smithing slots
			if (ctx.open_container != nullptr) {
				opened_inventory = ctx.open_container;
				opened_entity = dragon;
				inventory_open = true;
			}
		}
		break;
	}
	case Action::Bye:
		close_dialogue();
		break;
	}
}

void PlayMode::close_dialogue() {
	dialogue = DialogueBox{};
}

//Splits 'text' into lines no wider than 'max_width' (in text heights, see text_width). "\n" always breaks.
static std::vector<std::string> wrap_text(std::string const &text, float max_width) {
	std::vector<std::string> out;
	std::stringstream paragraphs(text);
	std::string paragraph;
	while (std::getline(paragraphs, paragraph)) {
		std::stringstream words(paragraph);
		std::string word, current;
		while (words >> word) {
			std::string longer = current.empty() ? word : current + " " + word;
			if (!current.empty() && text_width(longer) > max_width) {
				out.push_back(current);
				current = word;
			} else {
				current = longer;
			}
		}
		out.push_back(current); //(an empty paragraph is a blank line)
	}
	return out;
}

void PlayMode::draw_dialogue(Sprites &sprites, DrawLines &lines, glm::uvec2 drawable_size, float slot) {
	if (!dialogue.open || dialogue.pages.empty()) return;
	glm::vec2 screen = glm::vec2(drawable_size);

	float text_h = std::floor(slot * 0.38f);
	float line_h = std::floor(text_h * 1.45f);
	float name_h = std::floor(text_h * 1.1f);
	float pad = std::floor(slot * 0.4f);
	float border = std::max(2.0f, std::floor(slot * 0.12f));
	float panel_w = std::floor(std::min(screen.x - 2.0f * slot, slot * 20.0f));
	float inner_w = panel_w - 2.0f * pad;

	std::vector<std::string> wrapped = wrap_text(dialogue.pages[dialogue.page], inner_w / text_h);
	bool show_options = dialogue_options_visible();

	//options are buttons laid out left to right, wrapping onto more rows (keeps the box short so it
	//doesn't cover whoever is talking). Positions relative to the top-left of the options area:
	float button_h = std::floor(text_h * 1.6f);
	float button_pad = std::floor(text_h * 0.5f);
	float gap = std::floor(pad * 0.4f);
	std::vector<Rect> buttons;
	std::vector<std::string> labels;
	float options_h = 0.0f;
	if (show_options) {
		float bx = 0.0f, by = 0.0f;
		for (size_t i = 0; i < dialogue.options.size(); ++i) {
			std::string label = std::to_string(i + 1) + ". " + dialogue.options[i].label;
			float w = std::min(inner_w, std::ceil(text_width(label) * text_h) + 2.0f * button_pad);
			if (bx > 0.0f && bx + w > inner_w) { //next row
				bx = 0.0f;
				by += button_h + gap;
			}
			buttons.push_back(Rect{bx, by, w, button_h});
			labels.push_back(label);
			bx += w + gap;
		}
		options_h = by + button_h + std::floor(pad * 0.6f);
	}

	//panel: name, text, options; its bottom just above the hotbar
	float panel_h = pad + name_h + std::floor(line_h * 0.4f) + float(wrapped.size()) * line_h + options_h + pad;
	float bottom = std::floor(screen.y - slot * 1.9f);
	Rect panel{std::floor((screen.x - panel_w) * 0.5f), bottom - panel_h, panel_w, panel_h};
	dialogue.panel_rect = panel;

	sprites.fill_rect(Rect{panel.x - border, panel.y - border, panel.w + 2.0f * border, panel.h + 2.0f * border}, BOOK_COVER);
	sprites.fill_rect(panel, BOOK_PAPER);

	auto text = [&](std::string const &str, float tx, float ty, float h, glm::u8vec4 color) {
		lines.draw_text(str, glm::vec3(tx, ty, 0.0f), glm::vec3(h, 0.0f, 0.0f), glm::vec3(0.0f, -h, 0.0f), color);
	};

	//speaker, and the last message (e.g. "Got: Axe") on the right of the same line
	float x = panel.x + pad;
	float y = panel.y + pad + name_h;
	text(dialogue.speaker, x, y, name_h, BOOK_RED);
	if (!message.empty()) {
		text(message, panel.x + panel.w - pad - text_width(message) * text_h, y, text_h, BOOK_FADED);
	}
	y += std::floor(line_h * 0.4f);

	//the page, typed out so far
	size_t budget = size_t(dialogue.shown_chars);
	for (std::string const &row : wrapped) {
		y += line_h;
		if (budget == 0) continue;
		std::string shown = row.substr(0, std::min(budget, row.size()));
		budget -= std::min(budget, row.size() + 1); //+1 for the space or line break it was wrapped at
		text(shown, x, y - (line_h - text_h) * 0.5f, text_h, BOOK_INK);
	}

	//options, or a blinking arrow when there's another page
	dialogue.option_rects.clear();
	if (show_options) {
		float mx, my;
		SDL_GetMouseState(&mx, &my);
		glm::vec2 mouse = glm::vec2(mx, my) * window_scale;
		y += std::floor(pad * 0.6f);
		float edge = std::max(1.0f, std::floor(slot / 16.0f));
		for (size_t i = 0; i < buttons.size(); ++i) {
			Rect r{x + buttons[i].x, y + buttons[i].y, buttons[i].w, buttons[i].h};
			dialogue.option_rects.push_back(r);
			bool hovered = r.contains(mouse);
			sprites.fill_rect(r, hovered ? BOOK_FADED : BOOK_SPINE);
			sprites.fill_rect(Rect{r.x + edge, r.y + edge, r.w - 2.0f * edge, r.h - 2.0f * edge}, hovered ? BOOK_SPINE : BOOK_PAPER);
			text(labels[i], r.x + button_pad, r.y + (r.h + text_h) * 0.5f - edge, text_h, BOOK_INK);
		}
	} else if (dialogue.shown_chars >= float(dialogue.pages[dialogue.page].size()) && std::fmod(hint_time, 1.0f) < 0.6f) {
		//little down-pointing triangle in the bottom right corner
		float a = std::floor(text_h * 0.7f);
		float ax = panel.x + panel.w - pad - a, ay = panel.y + panel.h - pad - a * 0.6f;
		float step = std::max(1.0f, std::floor(a / 8.0f));
		for (float row = 0.0f; row * step * 2.0f < a; row += 1.0f) {
			sprites.fill_rect(Rect{ax + row * step, ay + row * step, a - 2.0f * row * step, step}, BOOK_INK);
		}
	}
}

// =================== Inventory mouse UI ================= //

void PlayMode::layout_ui_slots(glm::uvec2 drawable_size, float slot) {
	glm::vec2 screen = glm::vec2(drawable_size);
	float step = slot * 1.2f; //slot size + gap
	auto centered_x = [&](size_t slots) {
		float row_width = step * float(slots - 1) + slot;
		return std::floor((screen.x - row_width) * 0.5f);
	};
	auto add_row = [&](Inventory &inv, size_t first, size_t count, glm::vec2 top_left) {
		for (size_t i = 0; i < count; ++i) {
			ui_slots.emplace_back(SlotView{&inv, first + i, Rect{top_left.x + float(i) * step, top_left.y, slot, slot}});
		}
	};

	ui_slots.clear();
	//hotbar first (the message is drawn relative to its first slot), along the bottom:
	float player_x = centered_x(HOTBAR_SLOTS);
	float hotbar_y = std::floor(screen.y - slot * 1.5f);
	add_row(inventory, 0, HOTBAR_SLOTS, glm::vec2(player_x, hotbar_y));

	float top_y = hotbar_y;
	if (inventory_open) {
		//backpack rows stacked above the hotbar, with a small gap
		for (size_t row = 0; row < BACKPACK_ROWS; row++) {
			float y = std::floor(hotbar_y - slot * 0.4f - step * float(BACKPACK_ROWS - row));
			add_row(inventory, HOTBAR_SLOTS * (row + 1), HOTBAR_SLOTS, glm::vec2(player_x, y));
			top_y = std::min(top_y, y);
		}
	}
	show_arrow = false;
	if (opened_inventory != nullptr) {
		//opened container: above the player's inventory
		Inventory &inv = *opened_inventory;
		float y = std::floor(top_y - slot * 0.9f - step);

		//machines: outputs are the take-only slots after the inputs
		size_t first_output = inv.size();
		for (size_t i = 1; i < inv.size(); ++i) {
			if (!inv.rule(i).can_insert) { first_output = i; break; }
		}
		bool has_arrow = (first_output < inv.size());

		float x = centered_x(inv.size() + (has_arrow ? 1 : 0));
		for (size_t i = 0; i < inv.size(); ++i) {
			if (has_arrow && i == first_output) {
				arrow_rect = Rect{x, y, slot, slot};
				show_arrow = true;
				x += step;
			}
			ui_slots.emplace_back(SlotView{&inv, i, Rect{x, y, slot, slot}});
			x += step;
		}
	}
}

PlayMode::SlotView const *PlayMode::slot_at(glm::vec2 window_pos) const {
	//window coordinates -> drawable pixels (Retina)
	glm::vec2 p = window_pos * window_scale;
	for (SlotView const &view : ui_slots) {
		Rect const &r = view.rect;
		if (p.x >= r.x && p.x < r.x + r.w && p.y >= r.y && p.y < r.y + r.h) return &view;
	}
	return nullptr;
}

bool PlayMode::mouse_over_ui() const {
	if (ui_mouse_down) return true;
	float mx, my;
	SDL_GetMouseState(&mx, &my);
	return slot_at(glm::vec2(mx, my)) != nullptr;
}

void PlayMode::show_slot_result(ItemCursor::Result result) {
	if (result == ItemCursor::Result::CantInsert) {
		message = "Can't put that there";
		message_timer = MESSAGE_SECONDS;
	} else if (result == ItemCursor::Result::CantExtract) {
		message = "Can't take from there";
		message_timer = MESSAGE_SECONDS;
	}
}

void PlayMode::return_cursor_items() {
	if (cursor.empty()) return;
	int left = cursor.return_to(inventory);
	if (left > 0) {
		//bag is full: drop the rest at the dwarf's feet
		entities.push_back(std::make_unique<ItemEntity>(cursor.held.first, left));
		entities.back()->position = player.position;
		entities.back()->collision_mask = 0x0; //walk-through
		message = "Your bag is full";
		message_timer = MESSAGE_SECONDS;
	}
	cursor.clear();
}

void PlayMode::close_container() {
	return_cursor_items();
	inventory_open = false;
	opened_inventory = nullptr;
	opened_entity = nullptr;
	//slot views may point into the closed container; they're rebuilt on the next draw
	ui_slots.clear();
	ui_mouse_down = false;
}

template <typename EntityClass>
void PlayMode::add_entity(std::vector<std::unique_ptr<Entity>> &list, glm::vec2 position, uint32_t uid) {
	list.push_back(std::make_unique<EntityClass>());
	list.back()->position = position;
	if (uid != 0) {
		list.back()->uid = uid;
	}


	std::vector<glm::ivec2> occupied_tiles;
	list.back()->get_occupied(&occupied_tiles);

	for (auto const &tile: occupied_tiles) {
		if (coord_to_entities.find(tile) != coord_to_entities.end()) {
			coord_to_entities.at(tile).emplace_back(list.back().get());
		} else {
			coord_to_entities[tile] = std::vector<Entity *>{ };
			coord_to_entities.at(tile).emplace_back(list.back().get());
		}
	}
}

void PlayMode::add_machine(std::vector<std::unique_ptr<Entity>> &list, glm::vec2 position, item_id_t machine_type, uint32_t uid) {

	size_t inventory_size = 10;
	if (machine_type == "brewer") inventory_size = 3;
	else if (machine_type == "furnace") inventory_size = 2;
	else if (machine_type == "crusher") inventory_size = 2;
	else if (machine_type == "chest") inventory_size = 10;
	else if (machine_type == "distiller") inventory_size = 3;
	else if (machine_type == "fermenter") inventory_size = 4;

	list.push_back(std::make_unique<Machine>(inventory_size, machine_type, variety % 4));
	list.back()->position = position;
	if (uid != 0) {
		list.back()->uid = uid;
	}

	std::vector<glm::ivec2> occupied_tiles;
	list.back()->get_occupied(&occupied_tiles);

	for (auto const &tile: occupied_tiles) {
		if (coord_to_entities.find(tile) != coord_to_entities.end()) {
			coord_to_entities.at(tile).emplace_back(list.back().get());
		} else {
			coord_to_entities[tile] = std::vector<Entity *>{ };
			coord_to_entities.at(tile).emplace_back(list.back().get());
		}
	}
}


// =================== Other helpers ================= //

glm::ivec2 PlayMode::find_tilemap_coord(glm::vec2 screen_pos, PixelCanvas &view_canvas, glm::vec2 camera_topleft) {
	// 1. window coordinates -> drawable pixels, because of Retina schenanigans
	screen_pos *= window_scale;

	// 2. undo the centering and scaling
	screen_pos = screen_pos / float(view_canvas.scale);

	// 3. to world pos, then grid
	glm::vec2 world_pos = screen_pos + camera_topleft;
	return Tilemap::tile_at_point(world_pos);
}

void PlayMode::rebuild_coord_to_entities() {
	coord_to_entities.clear();
	for (auto &entity : entities) {
		std::vector<glm::ivec2> occupied_tiles;
		entity->get_occupied(&occupied_tiles);

		for (auto const &tile: occupied_tiles) {
			if (coord_to_entities.find(tile) != coord_to_entities.end()) {
				coord_to_entities.at(tile).emplace_back(entity.get());
			} else {
				coord_to_entities[tile] = std::vector<Entity *>{ };
				coord_to_entities.at(tile).emplace_back(entity.get());
			}
		}
	}
}

void PlayMode::rebuild_uid_to_entities() {
	uid_to_entities.clear();
	for (auto &entity : entities) {
		uid_to_entities[entity->uid] = entity.get();
	}
}

 auto PlayMode::entity_at(glm::ivec2 tile) const -> Entity* {
	if (coord_to_entities.find(tile) != coord_to_entities.end()) {
		auto &here = coord_to_entities.at(tile);
		if (here.size() == 0) return nullptr;
		return here[0];
	}
	return nullptr;
}

void PlayMode::spawn_item(item_id_t const &id, int count, glm::vec2 position) {
	entities.push_back(std::make_unique<ItemEntity>(id, count));
	entities.back()->position = position;
}

// ********************************************************************* //
// ============================    SAVE    ============================= //
// ********************************************************************* //

void PlayMode::save() {
	/**
	 * FORMAT:
	 * <version> <quest_idx> <quest_req_count> [QUEST_REQS] <inventory_size> [INVENTORIES]  <entity_count> [SIDE_QUESTS]
	 * SIDE_QUESTS = <count> then <id> <state> per side quest that has started (state 1 = active, 2 = done).
	 *               Optional: a file without it (like the level file) has no side quests started.
	 * (each line = entity line)
	 *
	 */


	std::cout << "SAVING!\n";
	std::ofstream fs(data_path(savefile_path));
	if (!fs) {
		return;
	}
	// int version, quest_idx, quest_req_count;
	// (active_quest is -1 once every main quest is done: no requirements then)
	size_t quest_req_count = (active_quest >= 0 && size_t(active_quest) < quests.size()) ? quests[active_quest].required_items.size() : 0;
	fs << 1 << ' ' << active_quest << ' ' << quest_req_count;

	// [QUEST_REQS]
	for (size_t idx = 0; idx < quest_req_count; ++idx) {
		fs << ' ' << static_cast<int>(quest_goals_cleared[idx]);
	}

	// [INVENTORY_SIZE]
	fs << ' ' << inventory.size();
	for (int idx = 0; idx < int(inventory.size()); ++idx) {
		item_id_t item_id = inventory[idx].first;
		if (item_id == "") item_id = "null";
		fs << ' ' << item_id << ' ' << inventory[idx].second;
	}

	// <entity_count>
	fs << ' ' << entities.size() + 1;   // +1 for player, which is saved separately

	// [SIDE_QUESTS]
	fs << ' ' << side_quest_states.size();
	for (auto const &[id, state] : side_quest_states) {
		fs << ' ' << id << ' ' << static_cast<int>(state);
	}
	fs << '\n';
	{ // player
		std::string line = player.save();
		fs << line << '\n';
	}

	// Each entity!
	for (auto const &entity : entities) {
		std::string line = entity->save();
		fs << line << '\n';
	}

	std::cout << "Save done!\n";
}

// LOAD
void PlayMode::load() {

	uint32_t largest_uid = 0;

	std::ifstream fs(data_path(savefile_path));
	if (!fs.is_open()) {
		fs.open(data_path(initfile_path));
		if (!fs.is_open()) {
			throw std::runtime_error{"Unloadable savefile!"};
		}
	}

	std::string line;
	std::getline(fs, line);

	int version, quest_req_count, entity_count;
	{ // QUEST & INVENTORY
		quest_goals_cleared.clear();
		std::istringstream ss(line);
		ss >> version >> active_quest >> quest_req_count;
		for (int idx = 0; idx < quest_req_count; ++idx) {
			int flag;
			ss >> flag;
			quest_goals_cleared.emplace_back(static_cast<bool>(flag));
		}

		int inventory_size;
		ss >> inventory_size;
		if (inventory_size < 0 || size_t(inventory_size) != inventory.size()) {
			throw std::runtime_error{"loaded inventory size mismatches expected size"};
		}
		for (int idx = 0; idx < int(inventory_size); ++idx) {
			item_id_t item_id;
			int count;
			ss >> item_id >> count;
			if (item_id == "null") item_id = "";
			inventory.set_slot(idx, item_id, count);
		}
		ss >> entity_count;

		// [SIDE_QUESTS] (optional)
		side_quest_states.clear();
		size_t side_count = 0;
		if (ss >> side_count) {
			for (size_t idx = 0; idx < side_count; ++idx) {
				std::string id;
				int state = 0;
				if (!(ss >> id >> state)) break;
				if (state == 1) side_quest_states[id] = SideQuestState::Active;
				else if (state == 2) side_quest_states[id] = SideQuestState::Done;
			}
		}
		// done
	}

	entities.clear();
	for (int idx = 0; idx < entity_count; ++idx) {
		std::getline(fs, line);
		if (line == "") {
			throw std::runtime_error{"Number of entity lines is invalid"};
		}
		std::istringstream ss(line);
		uint32_t uid;
		item_id_t entity_type;
		item_id_t machine_type = "";
		if (!(ss >> uid >> entity_type)) {
			throw std::runtime_error{"couldn't read entity type & uid for line: " + line};
		}
		if (entity_type == "machine") {
			if (!(ss >> machine_type)) {
				throw std::runtime_error{"couldn't read machine for line: " + line};
			}
		}

		if (uid > largest_uid) {
			largest_uid = uid;
		}

		// starts adding entities
		if (entity_type == "machine") {
			add_machine(entities, glm::vec2{0.0f, 0.0f}, machine_type, uid);
		} else if (entity_type == "apple_tree") {
			add_entity<AppleTree>(entities, glm::vec2{0.0f, 0.0f}, uid);
		} else if (entity_type == "boulder") {
			add_entity<Boulder>(entities, glm::vec2{0.0f, 0.0f}, uid);
		} else if (entity_type == "copper_vein" || entity_type == "iron_vein" || entity_type == "snowgem_vein" || entity_type == "rock_vein" || entity_type == "quartz_vein") {
			add_entity<OreVein>(entities, glm::vec2{0.0f, 0.0f}, uid); //load() sets which ore
		} else if (entity_type == "dragon") {
			add_entity<Dragon>(entities, glm::vec2{0.0f, 0.0f}, uid);
		} else if (entity_type == "lichen_rock" || entity_type == "fae_grass" || entity_type == "flameflower" || entity_type == "lifedew_plant") {
			add_entity<Forageable>(entities, glm::vec2{0.0f, 0.0f}, uid); //load() sets which kind
		} else if (entity_type == "tree") {
			add_entity<Tree>(entities, glm::vec2{0.0f, 0.0f}, uid);
		} else if (entity_type == "vulmanid" || entity_type == "glaber" || entity_type == "ice_lizard") {
			add_entity<Creature>(entities, glm::vec2{0.0f, 0.0f}, uid); //load() sets which creature
		} else if (entity_type == "lumberjack" || entity_type == "ice_elf") {
			add_entity<Npc>(entities, glm::vec2{0.0f, 0.0f}, uid); //load() sets which npc
		} else if (entity_type == "bridge") {
			add_entity<Bridge>(entities, glm::vec2{0.0f, 0.0f}, uid); //load() sets which npc
			if (auto *bridge = dynamic_cast<Bridge *>(entities.back().get())) {
				if (side_quest_state(bridge->definition().side_quest) == SideQuestState::Done) {
					bridge->repair();
				}
			}
		} else if (entity_type == "woodpile" || entity_type == "chopping_stump") {
			add_entity<Prop>(entities, glm::vec2{0.0f, 0.0f}, uid); //load() sets which prop
		} else if (entity_type == "pipe") {
			add_entity<Pipe>(entities, glm::vec2{0.0f, 0.0f}, uid);
		} else if (entity_type == "item_entity") {
			add_entity<ItemEntity>(entities, glm::vec2{0.0f, 0.0f}, uid);
		} else if (entity_type == "golem") {
			add_entity<Golem>(entities, glm::vec2{0.0f, 0.0f}, uid);
		} else if (entity_type == "fuelbox") {
			add_entity<Machine>(entities, glm::vec2{0.0f, 0.0f}, uid);
		} else if (entity_type == "player") {
			player.load(line);
		} else {
			// broken
			throw std::runtime_error{"Unrecognized entity name!"};
		}

		if (entity_type != "player") {
			entities.back()->load(line);
		}
	}
	Entity::reset_uid_counter(largest_uid);
}