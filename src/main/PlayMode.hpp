#pragma once

#include "../starter/Mode.hpp"

#include "../rendering/Camera2D.hpp"
#include "../rendering/PixelCanvas.hpp"
#include "../rendering/Sprites.hpp"
#include "../rendering/Texture.hpp"
#include "../world/Player.hpp"
#include "../world/TileMap.hpp"
#include "../world/Entity.hpp"
#include "../inventory/Item.hpp"
#include "../world/Components.hpp"

#include <glm/glm.hpp>

#include <memory>
#include <string>

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
	void remove_objects(float elapsed);

	// Helper methods
	template <typename EntityClass>
	void add_entity(std::vector<std::unique_ptr<Entity>> &entities, glm::vec2 position);

	glm::ivec2 find_tilemap_coord(glm::vec2 screen_pos, PixelCanvas &canvas, glm::vec2 camera_topleft);

	//----- world -----
	Tilemap map;
	std::unique_ptr< Texture > tileset; // loaded from the path named in the map file
	Player player;
	std::vector<std::unique_ptr<Entity>> entities; // TODO: add tilebased query for entities, and use hashmap to speedup lookup.
	//----- inventory -----
	Inventory inventory;                     // the player's hotbar
	Inventory *opened_inventory = nullptr;   // container panel currently open (nullptr = none)
	Entity *opened_entity = nullptr;         // the entity that owns opened_inventory
	bool inventory_taking = false;  // switch the flow direction between inventory <-> opened inventory.
	int hotbar_selected = -1;   // -1 = unselected
	//----- input -----
	FrameInput input;

	//----- world ineraction -----
	IBreakable *currently_breaking = nullptr;    // the IBreakable that is the current target. nullptr for none
	float break_timer = 0.0f;
	struct MouseButton {
		int downs = 0;
		bool pressed = false;
		bool popped = false;
	} left_click, right_click;
	glm::ivec2 hovered_coord = glm::ivec2{0};

	//short message from the last interaction ("Your bag is full"), shown for a few seconds:
	std::string message;
	float message_timer = 0.0f;

	//----- rendering -----
	Camera2D camera;
	Sprites batch;
	Sprites ui_batch;
	PixelCanvas canvas;
	float window_scale = 1.0f;   // related to retina schenanigans

	bool show_debug = false; // toggled with F3
};