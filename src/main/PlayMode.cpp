#include "PlayMode.hpp"

#include "GameConfig.hpp"

#include "../starter/DrawLines.hpp"
#include "../starter/Load.hpp"
#include "../starter/data_path.hpp"
#include "../starter/gl_errors.hpp"

#include "../world/entities/DemoBox.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <string>

using GameConfig::TILE_SIZE;

static constexpr int PLAYER_FRAME_H = 32;

//----- inventory UI -----
//Slots shown per inventory row.
static constexpr int INVENTORY_SLOTS = 11;

//Draws one row of inventory slots plus the items in them.
// top_left: screen pixels; slot: slot size in screen pixels; step: distance from one slot's left edge to the next
static void draw_inventory_row(Sprites &batch, DrawLines &lines, Inventory const &inventory,
                               glm::vec2 top_left, float slot, float step) {
	for (int i = 0; i < INVENTORY_SLOTS; i++) {
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
		if (i >= INVENTORY_SLOTS) break;
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
	for (size_t i = 0; i < 4; ++i) {
		entities.push_back(std::make_unique<DemoBox>());
		entities.back()->position = glm::vec2((i + 2.5) * TILE_SIZE, 17.5 * TILE_SIZE);
	}
	// initialize the inventory with two apple and one powder
	opened_inventory = nullptr;
	inventory.emplace_back("apple", 2);
	inventory.emplace_back("apple_powder", 1);
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

	// Update player
	player.update(input, elapsed, map);


	// Handle interact here
	while (input.interact.downs == 1) {
		if (opened_inventory != nullptr) {
			opened_inventory = nullptr;
			break;
		}

		auto interacted_tile = player.facing_tile();
		for (auto &entity : entities) {
			auto grid_pos = world_to_grid(entity->position - glm::vec2{0.0f, 8.0f});
			if (grid_pos != interacted_tile) {
				continue;
			}
			// found
			if (auto* target = dynamic_cast<IInteractable*>(entity.get())) {
				IInteractable::Context ctx;
				ctx.flag = IInteractable::Context::InteractionType::None;
				target->on_interact(ctx);
				if (ctx.flag != IInteractable::Context::InteractionType::OpenInventory) {
					continue;
				}
				// Is an Open inventory call
				if (auto* inv_target = dynamic_cast<IInventoryHolder*>(entity.get())) {
					opened_inventory = inv_target->get_inventory();
				}
			}
		}
		break;
	}

	// Update entities
	for (auto &entity : entities) {
		entity->update(input, elapsed, map);
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

	{ //player:
		player.draw(batch);
	}

	{ //entities:
		for (auto const &entity : entities) {
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
					batch.fill_rect(Rect{float(tx * TILE_SIZE), float(ty * TILE_SIZE), float(TILE_SIZE), float(TILE_SIZE)},
						glm::u8vec4(0xff, 0x30, 0x30, 0x50));
				}
			}
		}
		//tile in front of the player in blue, collision box in yellow:
		glm::ivec2 ft = player.facing_tile();
		batch.fill_rect(Rect{float(ft.x * TILE_SIZE), float(ft.y * TILE_SIZE), float(TILE_SIZE), float(TILE_SIZE)},
			glm::u8vec4(0x40, 0x80, 0xff, 0x60));
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
		float row_width = step * float(INVENTORY_SLOTS - 1) + slot;
		//center horizontally
		float row_x = std::floor((screen.x - row_width) * 0.5f);

		glm::vec2 hotbar_top_left(row_x, std::floor(screen.y - slot * 1.5f));
		//opened container (chest): centered vertically in the window
		glm::vec2 container_top_left(row_x, std::floor((screen.y - slot) * 0.5f));

		DrawLines lines(screen_to_clip);
		ui_batch.begin(screen_to_clip);
		draw_inventory_row(ui_batch, lines, inventory, hotbar_top_left, slot, step);
		if (opened_inventory != nullptr) {
			draw_inventory_row(ui_batch, lines, *opened_inventory, container_top_left, slot, step);
		}
		ui_batch.end();
	}

	GL_ERRORS();
}