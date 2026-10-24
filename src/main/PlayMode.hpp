#pragma once

#include "../starter/Mode.hpp"

#include "../rendering/Camera2D.hpp"
#include "../rendering/PixelCanvas.hpp"
#include "../rendering/Sprites.hpp"
#include "../rendering/Texture.hpp"
#include "../world/Player.hpp"
#include "../world/Tilemap.hpp"

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

	//----- rendering -----
	Camera2D camera;
	Sprites batch;
	PixelCanvas canvas;

	bool show_debug = false; // toggled with F3
};
