#pragma once

#include "../starter/GL.hpp"

#include <glm/glm.hpp>

#include <string>

// A GPU texture loaded from a PNG, set up for pixel art.
struct Texture {
	explicit Texture(std::string const &png_path);
	~Texture();

	// Textures own a GL object, so they can't be copied:
	Texture(Texture const &) = delete;
	Texture &operator=(Texture const &) = delete;

	GLuint tex = 0;
	glm::uvec2 size = glm::uvec2(0); // in pixels
};
