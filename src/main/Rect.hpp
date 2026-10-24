#pragma once

#include <glm/glm.hpp>

// An axis-aligned rectangle in pixels. (x, y) is the TOP-left corner and y grows down
struct Rect {
	float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;
	glm::vec2 min() const { return glm::vec2(x, y); }  //top-left
	glm::vec2 max() const { return glm::vec2(x + w, y + h); }  //bottom-right
};
