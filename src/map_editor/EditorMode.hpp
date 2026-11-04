#pragma once

#include "../starter/Mode.hpp"

#include "../rendering/Camera2D.hpp"
#include "../rendering/PixelCanvas.hpp"
#include "../rendering/Sprites.hpp"
#include "../rendering/Texture.hpp"
#include "../world/Player.hpp"
#include "../world/TileMap.hpp"

#include <glm/glm.hpp>
#include <iostream>

#include <memory>
#include <string>

const size_t HISTORY_LIMIT = 20;

// The main gameplay mode: a tilemap you can walk around on.
struct EditorMode : Mode {
	EditorMode() = delete;
	EditorMode(std::string path);
	virtual ~EditorMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- world -----
	Tilemap map;
	std::unique_ptr< Texture > tileset; // loaded from the path named in the map file
	std::vector<std::unique_ptr<Entity>> entities; // TODO: add tilebased query for entities, and use hashmap to speedup lookup.

	//----- input -----

	//short message from the last interaction ("Your bag is full"), shown for a few seconds:
	std::string message;
	float message_timer = 0.0f;

	//----- rendering -----
	Sprites batch;
	Sprites ui_batch;
	PixelCanvas canvas;
	PixelCanvas palette_canvas;    //side panel

	bool show_debug = false; // toggled with F3

	struct Button {
		int downs = 0;
		bool pressed = false;
	} space, key_e, key_b, key_g;

	struct MouseButton {
		int downs = 0;
		bool pressed = false;
		bool popped = false;
		glm::vec2 pos;
	} left_click, right_click;
	


	// ------ Editor -------
	int scale_slider = 6; // Tilemap display scaling
	glm::uvec2 drawable_size_;
	glm::ivec2 just_drawn_on;
	glm::ivec2 tileset_select_topleft, tileset_select_size;  // Rect with uint
	glm::vec2 drag_anchor;
	glm::vec2 view_topleft;
	glm::ivec2 drawrect_topleft;
	glm::ivec2 drawrect_bottomright;
	std::vector<bool> layers_activated;
	size_t selected_layer = 1;
	std::string hint = "";

	// HISTORY, Memory eater!
	std::deque<std::vector<std::vector< int32_t >>> history;

	enum class ToolMode {
		PEN,
		RECT,
		DRAG
	} tool_mode;
	enum class InteractWith {
		PANEL,
		MAP
	} interact_with;
	bool dragging = false, erasing = false;

	// helpers
	float window_scale = 1.0f;
	glm::ivec2 find_tilemap_coord(glm::vec2 screen_pos, PixelCanvas &canvas, glm::vec2 camera_topleft) {
		// 1. window coordinates -> drawable pixels, because of Retina schenanigans
		screen_pos *= window_scale;

		// 2. undo the centering and scaling
		screen_pos = screen_pos / float(canvas.scale);

		// 3. to world pos, then grid
		glm::vec2 world_pos = screen_pos + camera_topleft;
		return Tilemap::tile_at_point(world_pos);
	}

	bool add_history_queued = false;
	void queue_add_history() {
		add_history_queued = true;
	}
	void add_history() {
		// brrrrrrrrr
		history.push_back(map.tile_layers);
		if (history.size() > HISTORY_LIMIT) {
			history.pop_front();
		}
	}
	bool load_history() {
		// drrrrrrrrr
		if (history.empty()) {
			return false;
		}
		map.tile_layers = history.back();
		history.pop_back();
		return true;
	}
};