#include "PlayMode.hpp"

#include "GameConfig.hpp"

#include "../starter/DrawLines.hpp"
#include "../starter/Load.hpp"
#include "../starter/data_path.hpp"
#include "../starter/gl_errors.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <string>

using GameConfig::TILE_SIZE;

//Player art: one row of frames, each FRAME_W x FRAME_H, in the order down, up, left, right.
// (Placeholder until the sprite/animation system replaces it.)
static Load< Texture > player_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/player.png"));
});
static constexpr int PLAYER_FRAME_W = 16;
static constexpr int PLAYER_FRAME_H = 24;

PlayMode::PlayMode() : map(data_path("maps/cave.map")) {
	tileset = std::make_unique< Texture >(data_path(map.tileset_path));
	map.validate_tileset(*tileset);

	//stand in the middle of the spawn tile, feet near its bottom:
	player.position = Tilemap::tile_center(glm::ivec2(map.spawn_tile)) + glm::vec2(0.0f, TILE_SIZE * 0.25f);
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
	glm::vec2 direction(0.0f);
	if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT])  direction.x -= 1.0f;
	if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) direction.x += 1.0f;
	if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP])    direction.y -= 1.0f; //y grows downward
	if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN])  direction.y += 1.0f;

	player.update(direction, elapsed, map);
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
		int frame = 0; //down
		if (player.facing.y < 0) frame = 1;      //up
		else if (player.facing.x < 0) frame = 2; //left
		else if (player.facing.x > 0) frame = 3; //right
		Rect src{float(frame * PLAYER_FRAME_W), 0.0f, float(PLAYER_FRAME_W), float(PLAYER_FRAME_H)};
		//round to whole pixels so the sprite never sits between pixels:
		glm::vec2 top_left = glm::round(player.position) - glm::vec2(PLAYER_FRAME_W * 0.5f, float(PLAYER_FRAME_H));
		batch.draw(*player_texture, src, top_left);
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
			+ "  [" + map.type_at(tile).name + "]"
			+ "  zoom " + std::to_string(canvas.scale) + "x"
			+ "  canvas " + std::to_string(canvas.size.x) + "x" + std::to_string(canvas.size.y);

		float H = 12.0f * float(canvas.scale) * 0.5f; //text height in screen pixels
		glm::vec3 anchor(H * 0.5f, H * 1.5f, 0.0f);
		//dark shadow, then white text on top:
		lines.draw_text(text, anchor + glm::vec3(2.0f, 2.0f, 0.0f), glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, -H, 0.0f), glm::u8vec4(0x00, 0x00, 0x00, 0xff));
		lines.draw_text(text, anchor, glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, -H, 0.0f), glm::u8vec4(0xff, 0xff, 0xff, 0xff));
	}

	GL_ERRORS();
}
