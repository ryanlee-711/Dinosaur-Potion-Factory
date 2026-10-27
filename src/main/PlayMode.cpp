#include "PlayMode.hpp"

#include "GameConfig.hpp"

#include "../starter/DrawLines.hpp"
#include "../starter/Load.hpp"
#include "../starter/data_path.hpp"
#include "../starter/gl_errors.hpp"

#include "../world/Components.hpp"
#include "../world/World.hpp"
#include "../world/entities/DemoBox.hpp"
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

using GameConfig::TILE_SIZE;

static constexpr int PLAYER_FRAME_H = 32;

//How long an interaction message (e.g. "Your bag is full") stays on screen, in seconds.
static constexpr float MESSAGE_SECONDS = 2.5f;

//An open container panel closes by itself once the dwarf's feet are farther than this
static constexpr float CONTAINER_CLOSE_DISTANCE = float(GameConfig::TILE_SIZE);

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
//Draws one row of inventory slots plus the items in them.
// top_left: screen pixels; slot: slot size in screen pixels; step: distance from one slot's left edge to the next
static void draw_inventory_row(Sprites &batch, DrawLines &lines, Inventory const &inventory,
                               glm::vec2 top_left, float slot, float step) {
	for (size_t i = 0; i < inventory.size(); i++) {
		glm::vec2 corner = top_left + glm::vec2(float(i) * step, 0.0f);
		float border = 0.05f * slot;
		batch.fill_rect(Rect{corner.x - border, corner.y - border, slot + 2.0f * border, slot + 2.0f * border},
			glm::u8vec4(0x66, 0x66, 0x66, 0xff));
		batch.fill_rect(Rect{corner.x, corner.y, slot, slot},
			glm::u8vec4(0x22, 0x22, 0x22, 0x80));
	}

	//items: icon scaled to fill the slot, count in the bottom-right corner
	float text_h = slot * 0.375f; //count text height in screen pixels
	int i = 0;
	for (auto const &[item_id, item_count] : inventory) {
		if (item_count == 0) { i++; continue; } //empty slot
		if (Item const *item = Item::get(item_id); item != nullptr) {
			glm::vec2 corner = top_left + glm::vec2(float(i) * step, 0.0f);
			batch.draw(*item->texture, Rect{0.0f, 0.0f, 16.0f, 16.0f}, Rect{corner.x, corner.y, slot, slot});

			glm::vec3 anchor(corner.x + slot * 0.875f, corner.y + slot, 0.0f);
			std::string text = std::to_string(item_count);
			lines.draw_text(text, anchor + glm::vec3(2.0f, 2.0f, 0.0f), glm::vec3(text_h, 0.0f, 0.0f), glm::vec3(0.0f, -text_h, 0.0f), glm::u8vec4(0x00, 0x00, 0x00, 0xff));
			lines.draw_text(text, anchor, glm::vec3(text_h, 0.0f, 0.0f), glm::vec3(0.0f, -text_h, 0.0f), glm::u8vec4(0xff, 0xff, 0xff, 0xff));
		}
		i++;
	}
}

PlayMode::PlayMode() : map(data_path("maps/demo.map")) {
	tileset = std::make_unique< Texture >(data_path("assets/tileset_new.png"));
	//stand in the middle of the spawn tile, feet near its bottom:
	player.position = Tilemap::tile_center(glm::ivec2{10, 10}) + glm::vec2(0.0f, TILE_SIZE * 0.25f);

	// ~~Demo~~
	// initialize several boxes for demonstration (entities).
	for (int i = 0; i < 4; i++) {
		entities.push_back(std::make_unique<DemoBox>());
		//stand each box on a tile, in a row along tile row 17:
		entities.back()->position = Tilemap::tile_base(glm::ivec2{i + 2, 17});
	}
	// initialize the inventory with two apple and one powder
	inventory.add("apple", 2);
	inventory.add("apple_powder", 1);
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	//one-off key presses go here; held keys (movement) are read in update().
	if (evt.type == SDL_EVENT_KEY_DOWN && evt.key.key == SDLK_F3) {
		show_debug = !show_debug;
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

	//everything entities can collide with this frame:
	World world{map, entities};

	// Update player
	player.update(input, elapsed, world);

	// Close panel if walked away
	if (opened_entity != nullptr
	 && player.hitbox().distance_to(opened_entity->hitbox()) > CONTAINER_CLOSE_DISTANCE) {
		opened_inventory = nullptr;
		opened_entity = nullptr;
	}

	// Handle interact (only on the first frame E is down):
	if (input.interact.downs == 1) {
		if (opened_inventory != nullptr) {
			//E again closes whatever is open
			opened_inventory = nullptr;
			opened_entity = nullptr;
		} else if (Entity *entity = find_interact_target(player.interact_box(), entities)) {
			//the entity decides what happens, we just act on what it asks for:
			auto *target = dynamic_cast< IInteractable * >(entity);
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
		}
	}

	// Update entities
	for (auto &entity : entities) {
		if (entity->removed) continue;
		entity->update(input, elapsed, world);
	}

	// Remove entities that asked to be removed this frame.
	if (opened_entity != nullptr && opened_entity->removed) {
		//its panel is open: close it before the inventory it points at is deleted
		opened_inventory = nullptr;
		opened_entity = nullptr;
	}
	std::erase_if(entities, [](std::unique_ptr< Entity > const &entity) { return entity->removed; });

	// Count down the on-screen message
	if (message_timer > 0.0f) {
		message_timer -= elapsed;
		if (message_timer <= 0.0f) message.clear();
	}

	// clear input direction
	input.direction = glm::vec2{0.0f, 0.0f};
}

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
		glm::vec2 screen = glm::vec2(drawable_size);

		float slot = 16.0f * float(canvas.scale);
		float step = slot * 1.2f; //slot size + gap
		auto centered_x = [&](size_t slots) {
			float row_width = step * float(slots - 1) + slot;
			return std::floor((screen.x - row_width) * 0.5f);
		};

		glm::vec2 hotbar_top_left(centered_x(inventory.size()), std::floor(screen.y - slot * 1.5f));

		DrawLines lines(screen_to_clip);
		ui_batch.begin(screen_to_clip);
		draw_inventory_row(ui_batch, lines, inventory, hotbar_top_left, slot, step);
		if (opened_inventory != nullptr) {
			//opened container (chest): centered in the window
			glm::vec2 container_top_left(centered_x(opened_inventory->size()), std::floor((screen.y - slot) * 0.5f));
			draw_inventory_row(ui_batch, lines, *opened_inventory, container_top_left, slot, step);
		}
		if (!message.empty()) {
			//interaction message, just above the hotbar's left end:
			float text_h = slot * 0.4f;
			glm::vec3 anchor(hotbar_top_left.x, hotbar_top_left.y - slot * 0.35f, 0.0f);
			lines.draw_text(message, anchor + glm::vec3(2.0f, 2.0f, 0.0f), glm::vec3(text_h, 0.0f, 0.0f), glm::vec3(0.0f, -text_h, 0.0f), glm::u8vec4(0x00, 0x00, 0x00, 0xff));
			lines.draw_text(message, anchor, glm::vec3(text_h, 0.0f, 0.0f), glm::vec3(0.0f, -text_h, 0.0f), glm::u8vec4(0xff, 0xff, 0xff, 0xff));
		}
		ui_batch.end();
	}

	GL_ERRORS();
}