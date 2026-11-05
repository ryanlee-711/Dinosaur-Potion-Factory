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
#include "../world/entities/Chest.hpp"
#include "../world/entities/Pipe.hpp"
#include "../world/entities/Crusher.hpp"
#include "../world/entities/Cauldron.hpp"
#include "../world/entities/AppleTree.hpp"
#include "../world/entities/LichenRock.hpp"
#include "../world/entities/Boulder.hpp"
#include "../world/entities/Furnace.hpp"
#include "../world/entities/CopperVein.hpp"
#include "../world/entities/Dragon.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include <random>
#include <sstream>

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


// DRAGON DIALOGUES
static std::vector<std::vector<std::string>> DRAGON_DIALOGUES {

	std::vector<std::string> {
		"Cough cough... I'm sick...\nplease bring me medicine...\nBrew potion...",
		"The cave collapsed... \ncough, because, ...\nmy health issues... *ACHOOOO*.",
		"Your pickaxe is missing... \nwhere is it...?",
		"If a rock is too heavy...\nDrink some ale to bulk up...",
	},
	std::vector<std::string> {
		"You've brewed Ale... \ndrink it and find your pickaxe...\nAnd bring back the furnace...",
		"You can press `interact`\nto push a rock...\nafter a bottle of drink.",
	},
	std::vector<std::string> {
		"Collect some copper ore by mining...\nAnd give it to me, to smith pipes.",
	},
	std::vector<std::string> {
		"Furnace is very powerful...\nDry the lichens... and grind them.",
		"Lichen powder yields a red solution...\nAdd apples to it...\nto brew stronger potions...",
		"I can smith new machines for you...\nBring materials.",
	},
	std::vector<std::string> {
		"You've mastered brewing...Please bring me 8 Strength potion.\n(Smithing recipe)\nApple+Copper*3=Brewer\nLichen+Copper*3=Furnace\nRock+Copper*3=Crusher\nRock*3=Chest",
	},
};




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

	// ~~Demo~~
	// initialize some existing machines for demonstration (entities).

	for (size_t i = 0; i < 3; ++i) {
		add_entity<Pipe>(entities, glm::vec2((1 * i + 138.0f) * TILE_SIZE, 82.0f * TILE_SIZE));
	}
	{
		add_entity<Crusher>(entities, glm::vec2((3.0f + 138.0f) * TILE_SIZE, 82.0f * TILE_SIZE));
		add_entity<Cauldron>(entities, glm::vec2((9.0f + 138.0f) * TILE_SIZE, 82.0f * TILE_SIZE));
		add_entity<Chest>(entities, glm::vec2((3.0f * 4.0f + 138.0f) * TILE_SIZE, 78.0f * TILE_SIZE));
		add_entity<Chest>(entities, glm::vec2((3.0f * 3 + 137.0f) * TILE_SIZE, 75.0f * TILE_SIZE));
		add_entity<Furnace>(entities, Tilemap::tile_topleft(glm::ivec2(92, 65)));

		// Apple trees
		add_entity<AppleTree>(entities, glm::vec2((138.0f) * TILE_SIZE, 90.0f * TILE_SIZE));
		add_entity<AppleTree>(entities, glm::vec2((142.0f) * TILE_SIZE, 93.0f * TILE_SIZE));

		// more chests
		add_entity<Chest>(entities, Tilemap::tile_topleft(glm::ivec2(132, 89)));
		add_entity<Chest>(entities, Tilemap::tile_topleft(glm::ivec2(132, 91)));
		add_entity<Chest>(entities, Tilemap::tile_topleft(glm::ivec2(132, 93)));

		// Add one item entity somewhere
		entities.push_back(std::make_unique<ItemEntity>("apple", 3));
		entities.back()->position = glm::vec2((140.0f) * TILE_SIZE, 94.0f * TILE_SIZE);

		// Boulders scattered around
		add_entity<CopperVein>(entities, Tilemap::tile_topleft(glm::ivec2(122, 85)));
		add_entity<Boulder>(entities, Tilemap::tile_topleft(glm::ivec2(124, 84)));
		add_entity<Boulder>(entities, Tilemap::tile_topleft(glm::ivec2(121, 78)));
		add_entity<Boulder>(entities, Tilemap::tile_topleft(glm::ivec2(144, 91)));
		add_entity<CopperVein>(entities, Tilemap::tile_topleft(glm::ivec2(168, 63)));
		add_entity<Boulder>(entities, Tilemap::tile_topleft(glm::ivec2(176, 68)));
		add_entity<Boulder>(entities, Tilemap::tile_topleft(glm::ivec2(180, 69)));
		add_entity<Boulder>(entities, Tilemap::tile_topleft(glm::ivec2(178, 70)));
		add_entity<Boulder>(entities, Tilemap::tile_topleft(glm::ivec2(177, 72)));
		add_entity<Boulder>(entities, Tilemap::tile_topleft(glm::ivec2(179, 73)));
	}

	{ // boulders, and pickaxe stuck near it
		add_entity<Boulder>(entities, Tilemap::tile_topleft(glm::ivec2(104, 65)));
		add_entity<Boulder>(entities, Tilemap::tile_topleft(glm::ivec2(105, 67)));
		add_entity<Boulder>(entities, Tilemap::tile_topleft(glm::ivec2(101, 69)));
		add_entity<CopperVein>(entities, Tilemap::tile_topleft(glm::ivec2(103, 70)));
		add_entity<Boulder>(entities, Tilemap::tile_topleft(glm::ivec2(105, 71)));
		add_entity<Boulder>(entities, Tilemap::tile_topleft(glm::ivec2(104, 73)));

		// two small boulders, pushable...
		entities.push_back(std::make_unique<Boulder>(false));
		entities.back()->position = Tilemap::tile_topleft(glm::ivec2(105, 68));

		entities.push_back(std::make_unique<Boulder>(false));
		entities.back()->position = Tilemap::tile_topleft(glm::ivec2(106, 69));

		entities.push_back(std::make_unique<ItemEntity>("pickaxe", 1));
		entities.back()->position = Tilemap::tile_topleft(glm::ivec2(102, 67));

		entities.push_back(std::make_unique<ItemEntity>("rock", 40));
		entities.back()->position = Tilemap::tile_topleft(glm::ivec2(96, 65));
		entities.push_back(std::make_unique<ItemEntity>("rock", 35));
		entities.back()->position = Tilemap::tile_topleft(glm::ivec2(95, 59));
	}
	{ // Dragon
		add_entity<Dragon>(entities, Tilemap::tile_topleft(glm::ivec2(132, 49)));
	}

	{ // boulders on the left, and lichen rock
		add_entity<Boulder>(entities, Tilemap::tile_topleft(glm::ivec2(96, 56)));

		add_entity<Boulder>(entities, Tilemap::tile_topleft(glm::ivec2(94, 60)));
		add_entity<LichenRock>(entities, Tilemap::tile_topleft(glm::ivec2(91, 67)));
		add_entity<Boulder>(entities, Tilemap::tile_topleft(glm::ivec2(91, 70)));
		add_entity<Boulder>(entities, Tilemap::tile_topleft(glm::ivec2(97, 77)));
		add_entity<LichenRock>(entities, Tilemap::tile_topleft(glm::ivec2(88, 75)));

		add_entity<LichenRock>(entities, Tilemap::tile_topleft(glm::ivec2(92, 76)));
	}

	// recipe book: one page per recipe that makes a potion
	for (Recipe const *recipe : Recipe::all()) {
		if (recipe->items_out.empty()) continue;
		if (Item::tags_of(recipe->items_out[0].first) & ItemTag::Potion) book_recipes.push_back(recipe);
	}

	// initialize the inventory with two apple and one powder
	inventory.add("water_bottle", 15);

	// initialize quest
	quests.emplace_back(
		Quest{"Collect apples, crush them, and brew ale.\nE: interact, F: drink, Shift: sprint, Tab: Open Inventory",
			std::vector<ItemRecord>{ItemRecord{"apple", 6}, ItemRecord{"crushed_apple", 3}, ItemRecord{"ale", 1}},
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
		Quest{"Brew stronger potion from dried lichen and apple crush.",
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

}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	//one-off key presses go here; held keys (movement) are read in update().
	if (evt.type == SDL_EVENT_KEY_DOWN && evt.key.key == SDLK_F3) {
		show_debug = !show_debug;
		return true;
	}
	if (evt.type == SDL_EVENT_KEY_DOWN && evt.key.key == SDLK_F) {
		//F: drink / eat the selected consumable
		if (!inventory_open) consume_selected();
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
	//read held movement keys (scancodes = physical key positions, so WASD works on any keyboard layout):
	bool const *keys = SDL_GetKeyboardState(nullptr);
	if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT])  input.direction.x -= 1.0f;
	if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) input.direction.x += 1.0f;
	if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP])    input.direction.y -= 1.0f; //y grows downward
	if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN])  input.direction.y += 1.0f;
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
		input.destroy.pressed = true;
		input.destroy.downs += 1;
	} else {
		input.destroy.pressed = false;
		input.destroy.downs = 0;
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
	World world{map, entities, player, coord_to_entities, &spawned, &made_items};

	// Update player
	player.update(input, elapsed, world);

	// Close panel if walked away
	if (opened_entity != nullptr
	 && player.hitbox().distance_to(opened_entity->hitbox()) > CONTAINER_CLOSE_DISTANCE) {
		close_container();
	}

	// Rebuild entity mapping (before world interaction)
	rebuild_coord_to_entities();

	// Handle interact (only on the first frame E is down):
	do_world_interaction(elapsed);

	if (input.destroy.downs == 1) {
		glm::ivec2 target_tile = player.facing_tile();
		Entity *target = world.entity_at(target_tile);
		if (auto breakable = dynamic_cast< IInteractable * >(target); breakable != nullptr) {
			target->removed = true;
		}

	}

	// numkeys select/unselect hotbar items (1..9, 0 = tenth slot).
	// Moving items between inventories is done with the mouse (see handle_event).
	for (size_t raw_index = 0; raw_index < 10 && !inventory_open; raw_index++) {
		int index = int((raw_index + 9) % 10);
		if (input.numkeys[raw_index].downs != 1) continue;
		if (hotbar_selected == index) hotbar_selected = -1;
		else hotbar_selected = index;
		break;
	}
	// change Player's held item based on selection
	player.held_item = inventory[hotbar_selected];

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

	// bool right_clicked = (buttons & SDL_BUTTON_RMASK) != 0 || right_click.latched;
	// right_click.latched = false;
	// if (right_clicked) {
	// 	right_click.downs += 1;
	// 	right_click.pressed = true;
	// } else {
	// 	right_click.downs = 0;
	// 	right_click.pressed = false;
	// 	right_click.popped = true;
	// }

	// The inventory UI owns the mouse while it points at a slot or a press on a slot is still held.
	// While an inventory panel is open, the hotbar can't be used in the world: left click doesn't
	// place/use/break anything (the mouse is for moving items). Right click and E / Space can
	// still interact.
	bool over_ui = mouse_over_ui();
	bool panel_open = inventory_open || book_open;
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
			spawn_item("water_bottle", 1, player.position);
		}
	}

	// Hold left click with a tool: break the machine under the mouse (within TOOL_REACH)
	if (left_held && holding_tool && tile_in_reach(coord, TOOL_REACH)) {
		auto entity = entity_at(coord);
		auto *target = dynamic_cast< IBreakable * >(entity);
		if (target != nullptr && target == currently_breaking) {
			break_timer += elapsed;
			int damages = static_cast<int>(break_timer / SECOND_PER_DAMAGE);
			damages = damages > 5 ? 5 : damages;
			if (player.effect_level("strength") > 0) {
				damages *= player.effect_level("strength");
			}
			if (damages >= 5 && target->get_hardness() == 99 && target->get_drop_id() != "") {
				// these are non-dropping source blocks
				spawn_item(target->get_drop_id(), 1, entity->position);
				break_timer -= 5.0f * SECOND_PER_DAMAGE;
			}
			if (damages >= target->get_hardness()) {
				entity->removed = true;
			}
		} else {
			// changed breaking target or target is just null
			break_timer = 0.0f;
			currently_breaking = target;
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
		add_entity<Chest>(entities, world_pos);
	} else if (id == "crusher" && check_tile_empty(glm::ivec2{0, -1}, glm::ivec2{3, 0})) {
		add_entity<Crusher>(entities, world_pos);
	} else if (id == "brewer" && check_tile_empty(glm::ivec2{0, -1}, glm::ivec2{3, 0})) {
		add_entity<Cauldron>(entities, world_pos);
	} else if (id == "pipe") {
		add_entity<Pipe>(entities, world_pos);
	} else if (id == "furnace" && check_tile_empty(glm::ivec2{0, -1}, glm::ivec2{3, 0})) {
		add_entity<Furnace>(entities, world_pos);
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

	IInteractable::Context ctx{player, inventory};
	target->on_interact(ctx);
	if (ctx.open_container != nullptr) {
		opened_inventory = ctx.open_container;
		opened_entity = entity;
		inventory_open = true;
		book_open = false;
	}

	if (dynamic_cast< Dragon * >(entity)) {
		// If it is a dragon
		talking_to_dragon = true;
		dragon_dialogue_index += 1;
		clear_quest();
	} else {
		talking_to_dragon = false;
	}

	if (!ctx.message.empty()) {
		message = ctx.message;
		message_timer = MESSAGE_SECONDS;
	}
	return true;
}

void PlayMode::interact_facing() {
	//what's in front of the dwarf: the interactable overlapping the reach box the most,
	//else whatever occupies the facing tile
	Entity *target = find_interact_target(player.interact_box(), entities);
	if (target == nullptr) {
		Entity *in_front = entity_at(player.facing_tile());
		if (in_front != nullptr && dynamic_cast< IInteractable * >(in_front) != nullptr) target = in_front;
	}

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
	inventory.extract_from_slot(size_t(hotbar_selected), 1);
}

bool PlayMode::tile_in_reach(glm::ivec2 tile, float reach) const {
	return glm::length(glm::vec2(Tilemap::tile_at_point(player.position) - tile)) <= reach;
}

void PlayMode::remove_objects(float elapsed) {
	World world{map, entities, player, coord_to_entities};
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
		//the dwarf's reach (what E interacts with) in blue, collision box in yellow:
		batch.fill_rect(player.interact_box(), glm::u8vec4(0x40, 0x80, 0xff, 0xa0));
		batch.fill_rect(player.hitbox(), glm::u8vec4(0xff, 0xee, 0x00, 0xb0));
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

	//---- quest system ----
	if (active_quest != -1) {
		glm::mat4 screen_to_clip = glm::ortho(0.0f, float(drawable_size.x), float(drawable_size.y), 0.0f);
		DrawLines lines(screen_to_clip);
		draw_quest_ui(ui_batch, lines, drawable_size);
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

	//--- active effects, top left ---
	if (!player.active_effects().empty()) {
		glm::mat4 screen_to_clip = glm::ortho(0.0f, float(drawable_size.x), float(drawable_size.y), 0.0f);
		DrawLines lines(screen_to_clip);
		static char const *roman[] = {"", "I", " II", " III", " IV", " V"};
		float text_h = 8.0f * float(canvas.scale);
		float y = text_h * 1.5f;
		for (auto const &[id, active] : player.active_effects()) {
			Effect const *effect = Effect::get(id);
			std::string text = (effect ? effect->name : id)
				+ " " + (active.level < 6 ? roman[active.level] : " " + std::to_string(active.level))
				+ ":  " + std::to_string(int(std::ceil(active.time_left))) + "s";
			float x = std::floor(text_h * 0.5f);
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
	// only draw if there is an active quest
	if (active_quest < 0 || size_t(active_quest) > quests.size()) return;

	// right-anchored rendering
	float text_h = 8.0f * float(canvas.scale);
	float y = text_h * (1.5f + 1.4f * player.active_effects().size());

	// draw description
	std::stringstream ss("[Goals]  " + quests[active_quest].description);
    std::string line;

    while (getline(ss, line)) {
        float x = std::floor(text_h * 0.5f);
		glm::vec3 anchor(x, y, 0.0f);
		lines.draw_text(line, anchor + glm::vec3(2.0f, 2.0f, 0.0f), glm::vec3(text_h, 0.0f, 0.0f), glm::vec3(0.0f, -text_h, 0.0f), glm::u8vec4(0x00, 0x00, 0x00, 0xff));
		lines.draw_text(line, anchor, glm::vec3(text_h, 0.0f, 0.0f), glm::vec3(0.0f, -text_h, 0.0f), glm::u8vec4(0xff, 0xff, 0xf0, 0xff));
		y += text_h * 1.4f;
    }

	int req_idx = -1;
	bool all_satisfied = true;
	for (auto &record : quests[active_quest].required_items) {
		req_idx++;
		auto item = Item::get(record.first);
		int count = inventory.count(record.first);

		if (item == nullptr) continue;
		line = quest_goals_cleared[req_idx] ? "[ X ] " : "[   ] ";
		line += item->name + "  ";
		if (quest_goals_cleared[req_idx]) {
			line += std::to_string(record.second) + "/" + std::to_string(record.second);
		} else {
			line += std::to_string(count) + "/" + std::to_string(record.second);
			all_satisfied = false;
		}
		float x = std::floor(text_h * 0.5f);
		glm::vec3 anchor(x, y, 0.0f);
		glm::u8vec4 color = quest_goals_cleared[req_idx] ? glm::u8vec4(0xbf, 0xff, 0x90, 0xff) : glm::u8vec4(0xf0, 0xab, 0x90, 0xff);
		lines.draw_text(line, anchor + glm::vec3(2.0f, 2.0f, 0.0f), glm::vec3(text_h, 0.0f, 0.0f), glm::vec3(0.0f, -text_h, 0.0f), glm::u8vec4(0x00, 0x00, 0x00, 0xff));
		lines.draw_text(line, anchor, glm::vec3(text_h, 0.0f, 0.0f), glm::vec3(0.0f, -text_h, 0.0f), color);
		y += text_h * 1.4f;
	}

	// If finished, draw the "Talk to dragon" hint
	if (all_satisfied) {
		float x = std::floor(text_h * 0.5f);
		glm::vec3 anchor(x, y, 0.0f);
		glm::u8vec4 color = quest_goals_cleared[req_idx] ? glm::u8vec4(0xbf, 0xff, 0x90, 0xff) : glm::u8vec4(0xf0, 0xab, 0x90, 0xff);
		lines.draw_text("Talk to the dragon", anchor + glm::vec3(2.0f, 2.0f, 0.0f), glm::vec3(text_h, 0.0f, 0.0f), glm::vec3(0.0f, -text_h, 0.0f), glm::u8vec4(0x00, 0x00, 0x00, 0xff));
		lines.draw_text("Talk to the dragon", anchor, glm::vec3(text_h, 0.0f, 0.0f), glm::vec3(0.0f, -text_h, 0.0f), color);
		y += text_h * 1.4f;
	}

	// draw dragon dialogue
	if (talking_to_dragon) {
		float x = std::floor(drawable_size.x * 0.11f);
		y = drawable_size.y * 0.6f;
		glm::vec3 anchor(x, y, 0.0f);
		glm::u8vec4 color = glm::u8vec4(0xff, 0xa0, 0x90, 0xff);

		std::string text = "Dragon";
		lines.draw_text(text, anchor + glm::vec3(2.0f, 2.0f, 0.0f), glm::vec3(text_h, 0.0f, 0.0f), glm::vec3(0.0f, -text_h, 0.0f), glm::u8vec4(0x00, 0x00, 0x00, 0xff));
		lines.draw_text(text, anchor, glm::vec3(text_h, 0.0f, 0.0f), glm::vec3(0.0f, -text_h, 0.0f), color);

		x = std::floor(text_h * 0.5f);
		y += text_h * 2.4f;
		// dialogues
		if (active_quest >= 0 && size_t(active_quest) < DRAGON_DIALOGUES.size()) {
			 auto const &dialogues = DRAGON_DIALOGUES[active_quest];
			if (!dialogues.empty()) {
				std::stringstream ss2(dialogues[dragon_dialogue_index % dialogues.size()]);
				std::string dialogue_line;

				while (std::getline(ss2, dialogue_line)) {
					glm::vec3 anchor2(x, y, 0.0f);
					lines.draw_text(dialogue_line, anchor2 + glm::vec3(2, 2, 0), glm::vec3(text_h, 0, 0), glm::vec3(0, -text_h, 0), glm::u8vec4(0x00, 0x00, 0x00, 0xff));
					lines.draw_text(dialogue_line, anchor2, glm::vec3(text_h, 0, 0), glm::vec3(0, -text_h, 0), color);
					y += text_h * 1.4f;
				}
			}
		}

	}


}

void PlayMode::clear_quest() {
	// this function tries to clear the quest if it is clearable.
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
	talking_to_dragon = false;
}

template <typename EntityClass>
void PlayMode::add_entity(std::vector<std::unique_ptr<Entity>> &list, glm::vec2 position) {
	list.push_back(std::make_unique<EntityClass>());
	list.back()->position = position;

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