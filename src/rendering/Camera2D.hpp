#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// A 2D camera for the world
struct Camera2D {
	glm::vec2 center = glm::vec2(0.0f); // world point at the middle of the view

	// Center on 'target', but don't show past the edges of the world
	void follow(glm::vec2 target, glm::vec2 world_size, glm::uvec2 view_size) {
		glm::vec2 half = glm::vec2(view_size) * 0.5f;
		for (int axis = 0; axis < 2; ++axis) {
			if (world_size[axis] <= float(view_size[axis])) {
				center[axis] = world_size[axis] * 0.5f;
			} else {
				center[axis] = glm::clamp(target[axis], half[axis], world_size[axis] - half[axis]);
			}
		}
	}

	// Top-left world pixel of the view, snapped to whole pixels
	glm::vec2 view_min(glm::uvec2 view_size) const {
		return glm::floor(center - glm::vec2(view_size) * 0.5f);
	}

	glm::vec2 view_max(glm::uvec2 view_size) const {
		return view_min(view_size) + glm::vec2(view_size);
	}

	// Matrix for Sprites::begin: world pixels -> clip space (y down).
	glm::mat4 world_to_clip(glm::uvec2 view_size) const {
		glm::vec2 lo = view_min(view_size);
		glm::vec2 hi = lo + glm::vec2(view_size);
		return glm::ortho(lo.x, hi.x, hi.y, lo.y);
	}
};
