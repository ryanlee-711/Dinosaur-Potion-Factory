#include "PlayMode.hpp"

#include "GameConfig.hpp"

#include "../starter/DrawLines.hpp"
#include "../starter/Load.hpp"
#include "../starter/data_path.hpp"
#include "../starter/gl_errors.hpp"

#include "../world/Components.hpp"
#include "../world/World.hpp"
#include "../world/ItemEntity.hpp"
// Make macro for this?
#include "../world/entities/Chest.hpp"
#include "../world/entities/Pipe.hpp"
#include "../world/entities/Crusher.hpp"
#include "../world/entities/Cauldron.hpp"
#include "../world/entities/AppleTree.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include <random>

using GameConfig::TILE_SIZE;

static constexpr int PLAYER_FRAME_H = 32;

//How long an interaction message (e.g. "Your bag is full") stays on screen, in seconds.
static constexpr float MESSAGE_SECONDS = 2.5f;

//An open container panel closes by itself once the dwarf's feet are farther than this
static constexpr float CONTAINER_CLOSE_DISTANCE = float(GameConfig::TILE_SIZE);

//How long it takes to cause one damage by pickaxe to a machine
static constexpr float SECOND_PER_DAMAGE = 0.25f;

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

//----- inventory UI -----
//Draws an item icon filling 'rect' with its count in the bottom-right corner (screen pixels).
static void draw_item_stack(Sprites &batch, DrawLines &lines, ItemRecord const &record, Rect const &rect) {
	if (record.second <= 0) return;
	Item const *item = Item::get(record.first);
	if (item == nullptr) return;
	batch.draw(*item->texture, Rect{0.0f, 0.0f, 16.0f, 16.0f}, rect);

	float text_h = rect.w * 0.375f; //count text height in screen pixels
	glm::vec3 anchor(rect.x + rect.w * 0.875f, rect.y + rect.h, 0.0f);
	std::string text = std::to_string(record.second);
	lines.draw_text(text, anchor + glm::vec3(2.0f, 2.0f, 0.0f), glm::vec3(text_h, 0.0f, 0.0f), glm::vec3(0.0f, -text_h, 0.0f), glm::u8vec4(0x00, 0x00, 0x00, 0xff));
	lines.draw_text(text, anchor, glm::vec3(text_h, 0.0f, 0.0f), glm::vec3(0.0f, -text_h, 0.0f), glm::u8vec4(0xff, 0xff, 0xff, 0xff));
}

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
	// initialize several boxes for demonstration (entities).

	for (size_t i = 0; i < 3; ++i) {
		add_entity<Pipe>(entities, glm::vec2((1 * i + 138.0f) * TILE_SIZE, 82.0f * TILE_SIZE));
	}

	{
		add_entity<Crusher>(entities, glm::vec2((3.0f + 138.0f) * TILE_SIZE, 82.0f * TILE_SIZE));
		add_entity<Cauldron>(entities, glm::vec2((9.0f + 138.0f) * TILE_SIZE, 82.0f * TILE_SIZE));
		add_entity<Chest>(entities, glm::vec2((3.0f * 4.0f + 138.0f) * TILE_SIZE, 78.0f * TILE_SIZE));
	}
	{
		add_entity<Chest>(entities, glm::vec2((3.0f * 3 + 137.0f) * TILE_SIZE, 75.0f * TILE_SIZE));
		add_entity<AppleTree>(entities, glm::vec2((138.0f) * TILE_SIZE, 92.0f * TILE_SIZE));

	}
	{ // Add one item entity somewhere
		entities.push_back(std::make_unique<ItemEntity>("apple", 3));
		entities.back()->position = glm::vec2((140.0f) * TILE_SIZE, 94.0f * TILE_SIZE);
	}

	// initialize the inventory with two apple and one powder
	inventory.add("pickaxe", 1);
	inventory.add("ale", 20);
	inventory.add("crushed_apple", 10);
	inventory.add("water_bottle", 10);
	inventory.add("pipe", 10);
	inventory.add("chest", 10);
	inventory.add("crusher", 10);
	inventory.add("brewer", 10);
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	//one-off key presses go here; held keys (movement) are read in update().
	if (evt.type == SDL_EVENT_KEY_DOWN && evt.key.key == SDLK_F3) {
		show_debug = !show_debug;
		return true;
	}
	if (evt.type == SDL_EVENT_KEY_DOWN && evt.key.key == SDLK_ESCAPE) {
		//close the open container (and put anything on the cursor back in the bag)
		close_container();
		return true;
	}

	//----- inventory mouse controls (see ItemCursor.hpp for the full scheme) -----
	// Clicks are handled here, as events, so even very fast clicks are never missed.
	if (evt.type == SDL_EVENT_MOUSE_BUTTON_DOWN && evt.button.button == SDL_BUTTON_LEFT) {
		if (SlotView const *hit = slot_at(glm::vec2(evt.button.x, evt.button.y)); hit != nullptr) {
			if (opened_inventory == nullptr) {
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
	if (evt.type == SDL_EVENT_MOUSE_BUTTON_DOWN && evt.button.button == SDL_BUTTON_RIGHT) {
		right_click.latched = true;
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
	World world{map, entities, player, coord_to_entities};

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
	for (size_t raw_index = 0; raw_index < 10; ++raw_index) {
		int index = int((raw_index + 9) % 10);
		if (input.numkeys[raw_index].downs != 1) continue;
		if (hotbar_selected == index) hotbar_selected = -1;
		else hotbar_selected = index;
		break;
	}

	// Update entities
	for (auto &entity : entities) {
		if (entity->removed) continue;
		entity->update(input, elapsed, world);
	}

	// collect ItemEntities on ground by player
	{
		for (auto &entity : world.entities) {
			if (auto *target = dynamic_cast< ItemEntity * >(entity.get()); target) {
				// If the item is close enough, then the flag for absorption will be set true;
				// Then, we consume it
				if (!target->is_absorbable()) continue;
				auto target_inventory = target->get_inventory();
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

	bool right_clicked = (buttons & SDL_BUTTON_RMASK) != 0 || right_click.latched;
	right_click.latched = false;
	if (right_clicked) {
		right_click.downs += 1;
		right_click.pressed = true;
	} else {
		right_click.downs = 0;
		right_click.pressed = false;
		right_click.popped = true;
	}

	// The inventory UI owns the mouse while it points at a slot or a press on a slot is still held.
	// While an inventory panel is open, the hotbar can't be used in the world: left click doesn't
	// place/use/break anything (the mouse is for moving items). Right click and E / Space can
	// still interact.
	bool over_ui = mouse_over_ui();
	bool panel_open = (opened_inventory != nullptr);
	if (over_ui || panel_open) break_timer = 0.0f;
	bool left_new = !over_ui && !panel_open && left_click.downs == 1;
	bool left_held = !over_ui && !panel_open && left_click.downs > 1;
	bool right_new = !over_ui && right_click.downs == 1;

	item_id_t item_id = inventory.get_slot_id(hotbar_selected);
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

	if (glm::length(glm::vec2(Tilemap::tile_at_point(player.position) - coord)) <= 6) {
		// get left click target
		if ((left_new && item_id != "pickaxe") || right_new) {
			auto entity = entity_at(coord);

			if (!panel_open && hotbar_selected != -1 && item_id != "" && item_id != "pickaxe") {
				// cases A. player has a selected item. Try to use it
				glm::vec2 world_pos = coord * GameConfig::TILE_SIZE;
				if (item_id == "ale") {
					player.add_effect(0, 30.0f);
					inventory.remove("ale", 1);
				} else if (entity == nullptr) {
					if (place_machine(item_id, world_pos)) {
						inventory.remove(item_id, 1);
					}
				} else {
					message = "Action failed";
					message_timer = MESSAGE_SECONDS;
				}
			} else {
				interact_with(entity);
			}
		} else if (left_held) {
			// being held for more than one frames
			// start breaking things. But first, find entity
			auto entity = entity_at(coord);
			auto *target = dynamic_cast< IBreakable * >(entity);

			if (target != nullptr && target == currently_breaking && item_id == "pickaxe") {
				// keep breaking...
				break_timer += elapsed;
				int damages = static_cast<int>(break_timer / SECOND_PER_DAMAGE);
				if (damages >= target->get_hardness()) {
					// breaked
					entity->removed = true;
				}

			} else {
				// changed breaking target or target is just null!
				break_timer = 0.0f;
				currently_breaking = target;
			}
		}

		if (left_click.popped) {
			break_timer = 0.0f;
			// keep the currently_breaking pointer for continuous breaking.
			// let's dont think about stale pointers...
		}
	}

	left_click.popped = false;
	right_click.popped = false;
}

bool PlayMode::place_machine(item_id_t const &id, glm::vec2 world_pos) {
	if (id == "chest") {
		add_entity<Chest>(entities, world_pos);
	} else if (id == "crusher") {
		add_entity<Crusher>(entities, world_pos);
	} else if (id == "brewer") {
		add_entity<Cauldron>(entities, world_pos);
	} else if (id == "pipe") {
		add_entity<Pipe>(entities, world_pos);
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
	if (opened_inventory != nullptr && (target == nullptr || target == opened_entity)) {
		close_container();
		return;
	}
	interact_with(target); //(closes the old panel first if it's a different entity)
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
		batch.fill_rect(Tilemap::tile_rect(hovered_coord), glm::u8vec4(
			0xee,
			inventory.get_slot_id(hotbar_selected) == "pickaxe" ? 0x88 : glm::length(glm::vec2(Tilemap::tile_at_point(player.position) - hovered_coord)) > 6 ? 0x44 : 0xdd,
			0x44, 0x28));
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
				ui_batch.fill_rect(r, glm::u8vec4(0x22, 0x22, 0x22, 0x80));

				draw_item_stack(ui_batch, lines, view.inventory->slots[view.slot], r);

				//while holding items, tint the slots that won't accept them
				if (!cursor.empty() && !view.inventory->can_insert(view.slot, cursor.held.first)) {
					ui_batch.fill_rect(r, glm::u8vec4(0xaa, 0x22, 0x22, 0x60));
				}
			}

			if (!message.empty() && !ui_slots.empty()) {
				//interaction message, just above the hotbar's left end (hotbar slots come first in ui_slots):
				Rect const &first = ui_slots.front().rect;
				float text_h = slot * 0.4f;
				glm::vec3 anchor(first.x, first.y - slot * 0.35f, 0.0f);
				lines.draw_text(message, anchor + glm::vec3(2.0f, 2.0f, 0.0f), glm::vec3(text_h, 0.0f, 0.0f), glm::vec3(0.0f, -text_h, 0.0f), glm::u8vec4(0x00, 0x00, 0x00, 0xff));
				lines.draw_text(message, anchor, glm::vec3(text_h, 0.0f, 0.0f), glm::vec3(0.0f, -text_h, 0.0f), glm::u8vec4(0xff, 0xff, 0xff, 0xff));
			} else if (std::string sel_id = inventory.get_slot_id(hotbar_selected); !ui_slots.empty() && sel_id != "") {
				auto item = Item::get(sel_id);
				std::string text = item->name + (item->description != "" ? "  -  " + item->description : "");
				Rect const &first = ui_slots.front().rect;
				float text_h = slot * 0.4f;
				glm::vec3 anchor(first.x, first.y - slot * 0.35f, 0.0f);
				lines.draw_text(text, anchor + glm::vec3(2.0f, 2.0f, 0.0f), glm::vec3(text_h, 0.0f, 0.0f), glm::vec3(0.0f, -text_h, 0.0f), glm::u8vec4(0x00, 0x00, 0x00, 0xff));
				lines.draw_text(text, anchor, glm::vec3(text_h, 0.0f, 0.0f), glm::vec3(0.0f, -text_h, 0.0f), glm::u8vec4(0xff, 0xff, 0xff, 0xff));
			}
			ui_batch.end();
		} //(lines draw here, on top of the slots)

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

	GL_ERRORS();
}

// =================== Inventory mouse UI ================= //

void PlayMode::layout_ui_slots(glm::uvec2 drawable_size, float slot) {
	glm::vec2 screen = glm::vec2(drawable_size);
	float step = slot * 1.2f; //slot size + gap
	auto centered_x = [&](size_t slots) {
		float row_width = step * float(slots - 1) + slot;
		return std::floor((screen.x - row_width) * 0.5f);
	};
	auto add_row = [&](Inventory &inv, glm::vec2 top_left) {
		for (size_t i = 0; i < inv.size(); ++i) {
			ui_slots.emplace_back(SlotView{&inv, i, Rect{top_left.x + float(i) * step, top_left.y, slot, slot}});
		}
	};

	ui_slots.clear();
	//hotbar first (the message is drawn relative to its first slot), along the bottom:
	add_row(inventory, glm::vec2(centered_x(inventory.size()), std::floor(screen.y - slot * 1.5f)));
	if (opened_inventory != nullptr) {
		//opened container: centered in the window
		add_row(*opened_inventory, glm::vec2(centered_x(opened_inventory->size()), std::floor((screen.y - slot) * 0.5f)));
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
	opened_inventory = nullptr;
	opened_entity = nullptr;
	//slot views may point into the closed container; they're rebuilt on the next draw
	ui_slots.clear();
	ui_mouse_down = false;
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
		auto &entities = coord_to_entities.at(tile);
		if (entities.size() == 0) return nullptr;
		return entities[0];
	}
	return nullptr;
}