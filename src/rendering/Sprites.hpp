#pragma once

#include "Texture.hpp"
#include "../main/Rect.hpp"
#include "../starter/GL.hpp"

#include <glm/glm.hpp>

#include <vector>

// Draws lots of textured rectangles ("quads") cheaply.
//
// Quads are collected into one vertex buffer and sent to the GPU together
//
// Usage each frame:
//   batch.begin(camera.world_to_clip(view_size));
//   batch.draw(tileset, src_rect, dst_rect);
//   ...
//   batch.end();
//
// Quads are drawn in the order they are submitted (later = on top)
struct Sprites {
	Sprites();
	~Sprites();

	Sprites(Sprites const &) = delete;
	Sprites &operator=(Sprites const &) = delete;

	// Start a batch
	void begin(glm::mat4 const &world_to_clip);

	// Draw part of a texture into dst
	void draw(Texture const &texture, Rect src, Rect dst, glm::u8vec4 tint = glm::u8vec4(0xff));

	// Same as above but with its top-left at 'position'
	void draw(Texture const &texture, Rect src, glm::vec2 position, glm::u8vec4 tint = glm::u8vec4(0xff));

	// Solid colored rectangle
	void fill_rect(Rect dst, glm::u8vec4 color);

	// Takes a raw GL texture. A negative src.h flips the image vertically
	void draw_raw(GLuint tex, glm::uvec2 tex_size, Rect src, Rect dst, glm::u8vec4 tint);

	// Send everything left to the GPU. Call once per begin().
	void end();

private:
	struct Vertex {
		glm::vec2 position;
		glm::u8vec4 color;
		glm::vec2 tex_coord;
	};

	void flush();

	std::vector< Vertex > vertices;
	GLuint current_tex = 0;
	glm::mat4 world_to_clip = glm::mat4(1.0f);
	bool in_batch = false;

	GLuint vertex_buffer = 0;
	GLuint vertex_array = 0;
	GLuint white_tex = 0; // 1x1 white pixel
};
