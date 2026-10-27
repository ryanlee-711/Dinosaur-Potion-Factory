#pragma once

#include <glm/glm.hpp>

#include <algorithm>
#include <cmath>

// An axis-aligned rectangle in pixels. (x, y) is the TOP-left corner and y grows down
struct Rect {
	float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;
	glm::vec2 min() const { return glm::vec2(x, y); }  //top-left
	glm::vec2 max() const { return glm::vec2(x + w, y + h); }  //bottom-right

	// True if the two rectangles share some area
	// Rectangles that only touch along an edge do NOT overlap
	bool overlaps(Rect const &o) const {
		return x < o.x + o.w && o.x < x + w
		    && y < o.y + o.h && o.y < y + h;
	}

	// Shortest distance between the two rectangles' edges (0 if they touch or overlap).
	float distance_to(Rect const &o) const {
		float dx = std::max({0.0f, o.x - (x + w), x - (o.x + o.w)});
		float dy = std::max({0.0f, o.y - (y + h), y - (o.y + o.h)});
		return std::sqrt(dx * dx + dy * dy);
	}

	// Area shared by the two rectangles (0 if they don't overlap).
	float overlap_area(Rect const &o) const {
		float ow = std::min(x + w, o.x + o.w) - std::max(x, o.x);
		float oh = std::min(y + h, o.y + o.h) - std::max(y, o.y);
		return (ow > 0.0f && oh > 0.0f) ? ow * oh : 0.0f;
	}
};