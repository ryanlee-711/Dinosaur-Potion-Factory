#pragma once

#include "../starter/GL.hpp"
#include "../main/GameConfig.hpp"

#include <glm/glm.hpp>

struct Sprites;

// A small offscreen image framebuffer the world is drawn into at the art's
// real resolution, then blown up to the window by a whole-number factor.
//
// Each frame:
//   canvas.resize(drawable_size);   // picks zoom + canvas size for the window
//   canvas.bind();                  // draw the world after this...
//   ...
//   canvas.present(batch, drawable_size); // ...then copy it to the window
struct PixelCanvas {
	PixelCanvas();
	~PixelCanvas();

	PixelCanvas(PixelCanvas const &) = delete;
	PixelCanvas &operator=(PixelCanvas const &) = delete;

	// Choose the zoom and canvas size for this window size
	void resize(glm::uvec2 drawable_size);

	// Make the canvas the render target
	void bind();

	// Draw the canvas to the window, scaled up and centered
	void present(Sprites &batch, glm::uvec2 drawable_size);

	glm::uvec2 size = glm::uvec2(0); // canvas size in art pixels
	int scale = 1;                   // screen pixels per art pixel

	int target_height = GameConfig::TARGET_VIEW_HEIGHT;  // Default is 270

private:
	GLuint framebuffer = 0;
	GLuint color_tex = 0;
};
