#include "EditorMode.hpp"

#include "../main/GameConfig.hpp"

#include "../starter/DrawLines.hpp"
#include "../starter/Load.hpp"
#include "../starter/data_path.hpp"
#include "../starter/gl_errors.hpp"

#include "../world/Components.hpp"
#include "../world/World.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

using GameConfig::TILE_SIZE;

static constexpr int TILESET_W = 256;
static constexpr int TILESET_H = 256;
static constexpr int ERASE_INDEX = 255;

static Load< Texture > arrow_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/ui/arrows.png"));
});

static Load< Texture > editor_tool_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/ui/editor/editor_icons.png"));
});

static Load< Texture > editor_digit_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/ui/editor/editor_nums.png"));
});

EditorMode::EditorMode(std::string path) : map(data_path(path)) {
	tileset = std::make_unique< Texture >(data_path("assets/tileset_new.png"));

	// initialize window scale, OS specific
	int window_w, window_h, drawable_w, drawable_h;
	SDL_GetWindowSize(Mode::window, &window_w, &window_h);
	SDL_GetWindowSizeInPixels(Mode::window, &drawable_w, &drawable_h);
	window_scale = float(drawable_w) / float(window_w);

	drawable_size_ = glm::uvec2(drawable_w, drawable_h);
	drag_anchor = glm::vec2{0.0f};
	view_topleft = glm::vec2{0.0f};

	// some metadata
	for (size_t layer = 0; layer < map.num_layers; ++layer) {
		layers_activated.emplace_back(true);
	}
	just_drawn_on = glm::ivec2{-1, -1};

	// add first frame
}

EditorMode::~EditorMode() {
}

bool EditorMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	//one-off key presses go here; held keys (movement) are read in update().
	if (evt.type == SDL_EVENT_KEY_DOWN && evt.key.key == SDLK_F3) {
		show_debug = !show_debug;
		return true;
	}

	if (evt.type == SDL_EVENT_KEY_DOWN) {
		if (evt.key.key == SDLK_MINUS) {
			scale_slider = std::clamp(scale_slider - 1, 1, 30);
			canvas.target_height = std::clamp(1800 / scale_slider, 30, 1800);
			std::cout << "zoom out " << scale_slider << "\n";
		} else if (evt.key.key == SDLK_EQUALS) {
			scale_slider = std::clamp(scale_slider + 1, 1, 30);
			canvas.target_height = std::clamp(1800 / scale_slider, 30, 1800);
			std::cout << "zoom in " << scale_slider << "\n";
		} else if (evt.key.key == SDLK_E) {
			erasing = !erasing;
		} else if (evt.key.key == SDLK_R || evt.key.key == SDLK_U) {
			if (!left_click.pressed) {
				tool_mode = ToolMode::RECT;
			}
			
		} else if (evt.key.key == SDLK_B) {
			if (!left_click.pressed) {
				tool_mode = ToolMode::PEN;
			}
		} else if (evt.key.key == SDLK_RIGHTBRACKET) {
			selected_layer = (selected_layer + 1) % map.num_layers;
		} else if (evt.key.key == SDLK_LEFTBRACKET) {
			selected_layer = (selected_layer - 1) % map.num_layers;
		} else if (evt.key.key == SDLK_Z) {
			bool succ = load_history();
			if (succ) {
				std::cout << "undo" << std::endl;
			}
		} else if (evt.key.key == SDLK_SPACE) {
			// pass
		} else if (evt.key.key == SDLK_S) {
			map.save();
			hint = "Map saved!";
		}

		// numkeys
		const uint num_key[8] = {SDLK_0, SDLK_1, SDLK_2, SDLK_3, SDLK_4, SDLK_5, SDLK_6, SDLK_7};
		for (size_t idx = 0; idx < 8; ++idx) {
			if (evt.key.key == num_key[idx]) {
				layers_activated[idx] = !layers_activated[idx];
			}
		}
	}
	if (evt.type == SDL_EVENT_MOUSE_BUTTON_DOWN && evt.button.button == SDL_BUTTON_LEFT) {
		left_click.downs += 1;
		left_click.pressed = true;
		left_click.pos = glm::vec2(evt.button.x, evt.button.y);
		drag_anchor = view_topleft;
	} else if (evt.type == SDL_EVENT_MOUSE_BUTTON_DOWN && evt.button.button == SDL_BUTTON_RIGHT) {
		right_click.downs += 1;
		right_click.pressed = true;
		right_click.pos = glm::vec2(evt.button.x, evt.button.y);
	}

	// WHEN POP
	if (evt.type == SDL_EVENT_MOUSE_BUTTON_UP && evt.button.button == SDL_BUTTON_LEFT) {
		left_click.pressed = false;
		left_click.popped = true;
	} else if (evt.type == SDL_EVENT_MOUSE_BUTTON_UP && evt.button.button == SDL_BUTTON_RIGHT) {
		right_click.pressed = false;
	}



	return false;
}

void EditorMode::update(float elapsed) {
	//read held movement keys (scancodes = physical key positions, so WASD works on any keyboard layout):
	bool const *keys = SDL_GetKeyboardState(nullptr);

	// set dragging flag
	if (!dragging) {
		dragging = keys[SDL_SCANCODE_SPACE];
	}
	

	[[maybe_unused]]
	auto print_pos = [](glm::vec2 pos) {
		std::cout << pos.x << " " << pos.y << std::endl;
	};
	
	{ //handle left click.
		// This handle the event of left being clicked.
		// If it clicked the panels, then data is written about that;
		// If it clicked the map view, then it does other stuff accordingly.
		if (left_click.downs > 0) {
			if (left_click.pos.x * window_scale < float(drawable_size_.y) * 0.5f) {
				interact_with = InteractWith::PANEL;
				glm::vec2 panel_pos = left_click.pos * window_scale;
				float panel_width = float(drawable_size_.y / 2);
				tileset_select_topleft = Tilemap::tile_at_point(panel_pos * (float(TILESET_W) / panel_width));
				tileset_select_size = glm::ivec2{1};
			} else {
				interact_with = InteractWith::MAP;
				drawrect_topleft = find_tilemap_coord(left_click.pos, canvas, view_topleft);
				if (!dragging) {
					add_history();
				}
			}
		}
	}

	{ //mouse actions (held)
		float mouse_x, mouse_y;
		SDL_GetMouseState(&mouse_x, &mouse_y);

		if (left_click.pressed && interact_with == InteractWith::PANEL) {
			glm::vec2 panel_pos = glm::vec2{mouse_x, mouse_y} * window_scale;
			float panel_width = float(drawable_size_.y / 2);
			glm::vec2 tileset_select_pos = panel_pos * (float(TILESET_W) / panel_width);
			glm::ivec2 tileset_select_at = Tilemap::tile_at_point(tileset_select_pos);
			tileset_select_at = tileset_select_at - tileset_select_topleft + glm::ivec2{1};
			if (tileset_select_at.x <= 1) tileset_select_at.x = 1;
			if (tileset_select_at.y <= 1) tileset_select_at.y = 1;
			tileset_select_size = tileset_select_at;
		}

		if (interact_with == InteractWith::MAP) {
			if (left_click.pressed && (tool_mode == ToolMode::DRAG || dragging)) {
				// DRAGGING
				view_topleft.x = drag_anchor.x - (mouse_x - left_click.pos.x) * window_scale / canvas.scale;
				view_topleft.y = drag_anchor.y - (mouse_y - left_click.pos.y) * window_scale / canvas.scale;
			} else if (left_click.pressed && (tool_mode == ToolMode::PEN)) {
				// PEN
				// draw the tile on
				glm::ivec2 cursor_loc = find_tilemap_coord(glm::vec2{mouse_x, mouse_y}, canvas, view_topleft);
				if (cursor_loc != just_drawn_on) {
					just_drawn_on = cursor_loc;
					for (size_t dy = 0; dy < tileset_select_size.y; dy++) {
						for (size_t dx = 0; dx < tileset_select_size.x; dx++) {
							glm::ivec2 draw_to = cursor_loc + glm::ivec2{dx, dy};
							if (!map.in_bounds(draw_to)) continue;
							glm::ivec2 draw_with = tileset_select_topleft + glm::ivec2{dx, dy};

							int32_t wvalue = (selected_layer == 0 ? (erasing ? 0 : 0x80000001)
									 :(erasing ? ERASE_INDEX : draw_with.y * (TILESET_H / GameConfig::TILE_SIZE) + draw_with.x));
							
							map.tile_layers[selected_layer][
								map.size.x * draw_to.y + draw_to.x
							] = wvalue;  // <--- this is tile index
						}
					}
				}
			} else if (left_click.pressed && (tool_mode == ToolMode::RECT)) {
				// RECT
				glm::ivec2 cursor_loc = find_tilemap_coord(glm::vec2{mouse_x, mouse_y}, canvas, view_topleft);
				drawrect_bottomright = cursor_loc;				
			}
		}

		// -------- MOUSE RELEASE ---------

		if (interact_with == InteractWith::MAP && !dragging && tool_mode == ToolMode::RECT && left_click.popped) {
			// draw the rectangle!
			// allows different directions of dragging rectangle
			glm::ivec2 rect_min = glm::min(drawrect_topleft, drawrect_bottomright);
			glm::ivec2 rect_max = glm::max(drawrect_topleft, drawrect_bottomright);

			for (int y = rect_min.y; y <= rect_max.y; ++y) {
				for (int x = rect_min.x; x <= rect_max.x; ++x) {
					glm::ivec2 draw_to{x, y};
					if (!map.in_bounds(draw_to)) continue;
					glm::ivec2 draw_with = tileset_select_topleft + glm::ivec2{
						(x - drawrect_topleft.x) % tileset_select_size.x,
						(y - drawrect_topleft.y) % tileset_select_size.y
					};

					int32_t wvalue = (selected_layer == 0 ? (erasing ? 0 : 0x80000001)
									 :(erasing ? ERASE_INDEX : draw_with.y * (TILESET_H / GameConfig::TILE_SIZE) + draw_with.x));

					map.tile_layers[selected_layer][
						map.size.x * draw_to.y + draw_to.x
					] = wvalue;  // <--- this is tile index
				}
			}
			drawrect_topleft = glm::ivec2{0};
			drawrect_bottomright = glm::ivec2{-1};
		}
	}

	// late overwrite of dragging flag
	if (dragging) {
		dragging = keys[SDL_SCANCODE_SPACE] || left_click.pressed;
	}


	// clear mouse buttons
	left_click.downs = 0;
	left_click.popped = false;
	right_click.downs = 0;
}

void EditorMode::draw(glm::uvec2 const &drawable_size) {
	// Update drawable_size_
	drawable_size_ = drawable_size;
	// Flush
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	//--- world, drawn into the low-res canvas ---
	canvas.resize(drawable_size);

	glm::vec2 view_min = view_topleft;
	glm::vec2 view_max = view_topleft + glm::vec2(canvas.size);
	glm::mat4 map_world_to_clip = glm::ortho(view_min.x, view_max.x, view_max.y, view_min.y);

	canvas.bind();
	glClearColor(0.05f, 0.04f, 0.06f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	batch.begin(map_world_to_clip);

	map.draw(batch, *tileset, view_min, view_max);

	if (layers_activated[0]) {
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
	}

	if (tool_mode == ToolMode::PEN && !dragging) {
		float mouse_x, mouse_y;
		SDL_GetMouseState(&mouse_x, &mouse_y);
		glm::ivec2 cursor_loc = find_tilemap_coord(glm::vec2{mouse_x, mouse_y}, canvas, view_topleft);

		just_drawn_on = cursor_loc;
		for (size_t dy = 0; dy < tileset_select_size.y; dy++) {
			for (size_t dx = 0; dx < tileset_select_size.x; dx++) {
				glm::ivec2 draw_to = cursor_loc + glm::ivec2{dx, dy};
				if (!map.in_bounds(draw_to)) continue;
				glm::ivec2 draw_with = tileset_select_topleft + glm::ivec2{dx, dy};

				batch.draw(*tileset.get(), Tilemap::tile_rect(draw_with), glm::vec2(draw_to) * float(GameConfig::TILE_SIZE), glm::u8vec4{0x40});
			}
		}
		
	}

	if (tool_mode == ToolMode::RECT && left_click.pressed && !dragging) { // Highlighting rect
		glm::ivec2 rect_min = glm::min(drawrect_topleft, drawrect_bottomright);
		glm::ivec2 rect_max = glm::max(drawrect_topleft, drawrect_bottomright);
		for (int y = rect_min.y; y <= rect_max.y; ++y) {
			for (int x = rect_min.x; x <= rect_max.x; ++x) {
				glm::ivec2 draw_to{x, y};
				if (!map.in_bounds(draw_to)) continue;

				batch.fill_rect(Tilemap::tile_rect(draw_to), glm::u8vec4{0xff, 0xdd, 0x77, 0x80});
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

		glm::ivec2 tile = Tilemap::tile_at_point(view_topleft);
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

	//--- UI (screen pixels, laid out on another canvas) ---
	{
		// always take one-fifth of the drawable_size.y as width.
		glm::uvec2 panel_size = glm::uvec2(drawable_size.y / 2, drawable_size.y);
		palette_canvas.resize(panel_size);
		palette_canvas.target_height = 1800;

		palette_canvas.bind();
		glClearColor(0.05f, 0.02f, 0.04f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// begin batch
		glm::mat4 screen_to_clip = glm::ortho(0.0f, float(panel_size.x), float(panel_size.y), 0.0f);
		ui_batch.begin(screen_to_clip);
		
		ui_batch.draw(
			*tileset.get(),
			Rect{0.0f, 0.0f, float(TILESET_W), float(TILESET_H)},
			Rect{0.0f, 0.0f, float(panel_size.x), float(panel_size.x)},  // x * x
			glm::u8vec4{0xff}
		);

		// draw icons
		float icon_scale = float(panel_size.x) / float(TILESET_W);
		// drag
		ui_batch.draw(
			*editor_tool_texture,
			Rect{64.0f, dragging || tool_mode == ToolMode::DRAG ? 32.0f : 0.0f, 32.0f, 32.0f}, 
			Rect{0.0f, float(panel_size.x) * 1.1f, 32.0f * icon_scale, 32.0f * icon_scale},
			glm::u8vec4{0xff}
		);
		// pen
		ui_batch.draw(
			*editor_tool_texture,
			Rect{0.0f, tool_mode == ToolMode::PEN ? 32.0f : 0.0f, 32.0f, 32.0f}, 
			Rect{32.0f * icon_scale, float(panel_size.x) * 1.1f, 32.0f * icon_scale, 32.0f * icon_scale},
			glm::u8vec4{0xff}
		);

		// rect
		ui_batch.draw(
			*editor_tool_texture,
			Rect{32.0f, tool_mode == ToolMode::RECT ? 32.0f : 0.0f, 32.0f, 32.0f}, 
			Rect{32.0f * icon_scale * 2.0f, float(panel_size.x) * 1.1f, 32.0f * icon_scale, 32.0f * icon_scale},
			glm::u8vec4{0xff}
		);

		// erasing
		ui_batch.draw(
			*editor_tool_texture,
			Rect{96.0f, erasing ? 32.0f : 0.0f, 32.0f, 32.0f}, 
			Rect{32.0f * icon_scale * 3.0f, float(panel_size.x) * 1.1f, 32.0f * icon_scale, 32.0f * icon_scale},
			glm::u8vec4{0xff}
		);

		// draw some highlights for selection
		// FULL = Rect{0.0f, 0.0f, float(panel_size.x), float(panel_size.x)}
		ui_batch.fill_rect(
			Rect{
				tileset_select_topleft.x * GameConfig::TILE_SIZE * icon_scale,
				tileset_select_topleft.y * GameConfig::TILE_SIZE * icon_scale,
				GameConfig::TILE_SIZE * icon_scale * tileset_select_size.x,
				GameConfig::TILE_SIZE * icon_scale * tileset_select_size.y
			},
			glm::u8vec4{0x80}
		);
		// draw selection
		ui_batch.fill_rect(
			Rect{32.0f * float(selected_layer) * icon_scale, float(panel_size.x) * 1.3f, 32.0f * icon_scale, 32.0f * icon_scale},
			glm::u8vec4{0xff, 0xff, 0x44, 0xff}
		);

		// draw layer activation
		for (size_t idx = 0; idx < map.num_layers; ++idx) {
			ui_batch.draw(
			*editor_digit_texture,
			Rect{32.0f * float(idx), layers_activated[idx] ? 32.0f : 0.0f, 32.0f, 32.0f}, 
			Rect{32.0f * float(idx) * icon_scale, float(panel_size.x) * 1.3f, 32.0f * icon_scale, 32.0f * icon_scale},
			glm::u8vec4{0xff}
		);
		}

		ui_batch.end();
		palette_canvas.present(ui_batch, panel_size);
	}

	{
		// restore
		glViewport(0, 0, drawable_size.x, drawable_size.y);
		glm::mat4 screen_to_clip = glm::ortho(0.0f, float(drawable_size.x), float(drawable_size.y), 0.0f);
		DrawLines lines(screen_to_clip);

		float H = 12.0f * float(canvas.scale) * 0.5f; //text height in screen pixels
		glm::vec3 anchor(H * 0.5f, float(drawable_size.y) - H * 1.5f, 0.0f);
		//dark shadow, then white text on top:
		lines.draw_text(hint, anchor + glm::vec3(2.0f, 2.0f, 0.0f), glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, -H, 0.0f), glm::u8vec4(0x00, 0x00, 0x00, 0xff));
		lines.draw_text(hint, anchor, glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, -H, 0.0f), glm::u8vec4(0xff, 0xff, 0xff, 0xff));
	}

	GL_ERRORS();
}