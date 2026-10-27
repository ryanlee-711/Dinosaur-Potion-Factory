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

#include <glm/glm.hpp>

#include <memory>

// The main gameplay mode: a tilemap you can walk around on.
struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- world -----
	Tilemap map;
	std::unique_ptr< Texture > tileset; // loaded from the path named in the map file
	Player player;
	std::vector<std::unique_ptr<Entity>> entities; // TODO: add tilebased query for entities, and use hashmap to speedup lookup.
	//----- inventory -----
	Inventory inventory;
	Inventory *opened_inventory;
	bool inventory_taking = false;  // switch the flow direction between inventory <-> opened inventory.
	//----- input -----
	FrameInput input;

	//----- rendering -----
	Camera2D camera;
	Sprites batch;
	Sprites ui_batch;
	PixelCanvas canvas;

	bool show_debug = false; // toggled with F3
};
