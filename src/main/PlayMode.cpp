#include "PlayMode.hpp"

#include "GameConfig.hpp"

#include "../starter/DrawLines.hpp"
#include "../starter/Load.hpp"
#include "../starter/data_path.hpp"
#include "../starter/gl_errors.hpp"

// Make macro for this?
#include "../world/entities/Chest.hpp"
#include "../world/entities/Pipe.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <string>

using GameConfig::TILE_SIZE;

static constexpr int PLAYER_FRAME_H = 32;

static Load< Texture > arrow_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/ui/arrows.png"));
});

PlayMode::PlayMode() : map(data_path("maps/demo.map")) {
	tileset = std::make_unique< Texture >(data_path("assets/tileset_new.png"));
	//stand in the middle of the spawn tile, feet near its bottom:
	player.position = Tilemap::tile_center(glm::ivec2{10, 10}) + glm::vec2(0.0f, TILE_SIZE * 0.25f);

	// ~~Demo~~
	// initialize several boxes for demonstration (entities).
	for (size_t i = 0; i < 4; ++i) {
		entities.push_back(std::make_unique<Chest>());
		entities.back()->position = glm::vec2((3 * i + 17.0f) * TILE_SIZE, 17.0f * TILE_SIZE);
		entities.push_back(std::make_unique<Pipe>());
		entities.back()->position = glm::vec2((3 * i + 19.0f) * TILE_SIZE, 17.0f * TILE_SIZE);
	}


	// initialize the inventory with two apple and one powder
	opened_inventory = nullptr;
	inventory.emplace_back("apple", 20);
	inventory.emplace_back("apple_powder", 10);
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
	if (keys[SDL_SCANCODE_TAB]) {
		input.tab.pressed = true;
		input.tab.downs += 1;
	} else {
		input.tab.pressed = false;
		input.tab.downs = 0;
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
	
	// Update player
	player.update(input, elapsed, map, entities);
	
	
	// Handle interact here
	if (input.tab.downs == 1) {
		// flip
		inventory_taking = !inventory_taking;
	}
	while (input.interact.downs == 1) {
		if (opened_inventory != nullptr) {
			opened_inventory = nullptr;
			break;
		}

		auto interacted_tile = player.facing_tile();
		for (auto &entity : entities) {
			std::vector<glm::ivec2> occupied;
			entity->get_occupied(&occupied);

			bool found = false;
			for (auto tile : occupied) {
				if (interacted_tile == tile) {
					found = true;
					break;
				}
			}
			if (!found) continue;

			// found
			if (auto* target = dynamic_cast<IInteractable*>(entity.get())) {
				IInteractable::Context ctx;
				ctx.flag = IInteractable::Context::InteractionType::None;
				ctx.playmode = this;
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
	// transport items
	if (opened_inventory != nullptr && opened_inventory != &inventory) {
		Inventory &source = inventory_taking ? *opened_inventory : inventory;
		Inventory &destination = inventory_taking ? inventory : *opened_inventory;

		for (size_t raw_index = 0; raw_index < 10; ++raw_index) {
			size_t index = (raw_index + 9) % 10;
			if (input.numkeys[raw_index].downs != 1) continue;
			if (index >= source.size()) continue;
			if (source[index].second <= 0) continue;

			const auto item_id = source[index].first;

			auto existing = std::find_if(
				destination.begin(), destination.end(),
				[&](ItemRecord const &record) {
					return record.first == item_id;
				}
			);

			if (existing != destination.end()) {
				existing->second += 1;
			} else {
				if (destination.size() >= 10) continue;
				destination.emplace_back(item_id, 1);
			}

			source[index].second -= 1;
			if (source[index].second == 0) {
				source.erase(source.begin() + index);
			}

			break;
		}
	}

	// Update entities
	for (auto &entity : entities) {
		entity->update(input, elapsed, map, entities);
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

	// --- show inventory (using show_debug temporally as condition) ---
	glm::mat4 screen_to_clip = glm::ortho(0.0f, float(drawable_size.x), float(drawable_size.y), 0.0f);
	ui_batch.begin(screen_to_clip);
	if (true) {
		DrawLines lines(screen_to_clip);
		int idx = 0;
		const float GAP = 96.0f;
		const float ANCHOR_X = 800.5f;
		const float ANCHOR_Y = 1200.5f;

		// draw inventory background
		for (size_t j = 0; j <= 10; ++j) {
			glm::vec2 top_left = glm::vec2{ANCHOR_X + j * GAP, ANCHOR_Y};
			float slot_size = 16.0f * canvas.scale;
			ui_batch.fill_rect(Rect{float(top_left.x) - 0.05f * slot_size, float(top_left.y) - 0.05f * slot_size, slot_size * 1.1f, slot_size * 1.1f},
					glm::u8vec4(0x66, 0x66, 0x66, 0xff));
			ui_batch.fill_rect(Rect{float(top_left.x), float(top_left.y), slot_size, slot_size},
					glm::u8vec4(0x22, 0x22, 0x22, 0x80));
		}

		// draw inventory text and sprites
		for (const auto &[item_id, item_count] : inventory) {
			if (auto item = Item::get(item_id); item != nullptr) {
				glm::vec2 top_left = glm::vec2{ANCHOR_X + idx * GAP, ANCHOR_Y};
				float slot_size = 16.0f * canvas.scale;
				// draw sprite
				Rect src{0.0f, 0.0f, 16.0f, 16.0f};
				ui_batch.draw(*item->texture, src, Rect{top_left.x, top_left.y, slot_size, slot_size});
				// draw text
				float H = 12.0f * float(canvas.scale) * 0.5f; //text height in screen pixels
				glm::vec3 anchor(top_left.x + slot_size * 0.875f, top_left.y + slot_size, 0.0f);
				std::string text = std::to_string(item_count);
				lines.draw_text(text, anchor + glm::vec3(2.0f, 2.0f, 0.0f), glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, -H, 0.0f), glm::u8vec4(0x00, 0x00, 0x00, 0xff));
				lines.draw_text(text, anchor, glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, -H, 0.0f), glm::u8vec4(0xff, 0xff, 0xff, 0xff));
			}
			idx++;
		}
	}

	if (opened_inventory != nullptr) {
		DrawLines lines(screen_to_clip);
		int idx = 0;
		const float GAP = 96.0f;
		const float ANCHOR_X = 800.5f;
		const float ANCHOR_Y = 800.5f;

		// draw inventory direction indicator
		ui_batch.draw(
			*arrow_texture, Rect{inventory_taking ? 16.0f : 0.0f, 0.0f, 16.0f, 16.0f},
			Rect{ANCHOR_X - GAP, ANCHOR_Y, 16.0f * canvas.scale, 16.0f * canvas.scale}
		);
		// draw inventory background
		for (size_t j = 0; j <= 10; ++j) {
			glm::vec2 top_left = glm::vec2{ANCHOR_X + j * GAP, ANCHOR_Y};
			float slot_size = 16.0f * canvas.scale;

			ui_batch.fill_rect(Rect{float(top_left.x) - 0.05f * slot_size, float(top_left.y) - 0.05f * slot_size, slot_size * 1.1f, slot_size * 1.1f},
					glm::u8vec4(0x66, 0x66, 0x66, 0xff));
			ui_batch.fill_rect(Rect{float(top_left.x), float(top_left.y), slot_size, slot_size},
					glm::u8vec4(0x22, 0x22, 0x22, 0x80));
		}

		// draw inventory text and sprites
		for (const auto &[item_id, item_count] : *opened_inventory) {
			if (auto item = Item::get(item_id); item != nullptr) {
				glm::vec2 top_left = glm::vec2{ANCHOR_X + idx * GAP, ANCHOR_Y};
				float slot_size = 16.0f * canvas.scale;
				// draw sprite
				Rect src{0.0f, 0.0f, 16.0f, 16.0f};
				ui_batch.draw(*item->texture, src, Rect{top_left.x, top_left.y, slot_size, slot_size});
				// draw text
				float H = 12.0f * float(canvas.scale) * 0.5f; //text height in screen pixels
				glm::vec3 anchor(top_left.x + slot_size * 0.875f, top_left.y + slot_size, 0.0f);
				std::string text = std::to_string(item_count);
				lines.draw_text(text, anchor + glm::vec3(2.0f, 2.0f, 0.0f), glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, -H, 0.0f), glm::u8vec4(0x00, 0x00, 0x00, 0xff));
				lines.draw_text(text, anchor, glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, -H, 0.0f), glm::u8vec4(0xff, 0xff, 0xff, 0xff));
			}
			idx++;
		}
	}
	ui_batch.end();

	GL_ERRORS();
}
