#pragma once

// Game-wide constants
namespace GameConfig {
	// Size of one map tile, in pixels
	constexpr int TILE_SIZE = 16;

	// How many art pixels tall the visible world should be
	// (270 ~= 17 tiles tall)
	constexpr int TARGET_VIEW_HEIGHT = 270;
}
