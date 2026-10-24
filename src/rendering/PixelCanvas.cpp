#include "PixelCanvas.hpp"

#include "Sprites.hpp"
#include "../main/GameConfig.hpp"
#include "../starter/gl_errors.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <stdexcept>

PixelCanvas::PixelCanvas() {
	glGenFramebuffers(1, &framebuffer);
	glGenTextures(1, &color_tex);
}

PixelCanvas::~PixelCanvas() {
	glDeleteTextures(1, &color_tex);
	glDeleteFramebuffers(1, &framebuffer);
}

void PixelCanvas::resize(glm::uvec2 drawable_size) {
	drawable_size = glm::max(drawable_size, glm::uvec2(1));

	//whole-number zoom that makes the view closest to TARGET_VIEW_HEIGHT art pixels tall
	int new_scale = std::max(1, int(std::round(float(drawable_size.y) / float(GameConfig::TARGET_VIEW_HEIGHT))));

	//canvas just big enough to cover the window at that zoom
	glm::uvec2 new_size = (drawable_size + glm::uvec2(new_scale - 1)) / glm::uvec2(new_scale);

	if (new_scale == scale && new_size == size) return;
	scale = new_scale;
	size = new_size;

	glBindTexture(GL_TEXTURE_2D, color_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_tex, 0);
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		throw std::runtime_error("PixelCanvas: framebuffer is incomplete.");
	}

	GL_ERRORS();
}

void PixelCanvas::bind() {
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glViewport(0, 0, size.x, size.y);
}

void PixelCanvas::present(Sprites &batch, glm::uvec2 drawable_size) {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, drawable_size.x, drawable_size.y);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	//screen-pixel coordinates, y down
	glm::mat4 screen_to_clip = glm::ortho(0.0f, float(drawable_size.x), float(drawable_size.y), 0.0f);

	glm::vec2 scaled = glm::vec2(size) * float(scale);
	glm::vec2 offset = glm::floor((glm::vec2(drawable_size) - scaled) * 0.5f);

	batch.begin(screen_to_clip);
	//render-target textures store their bottom row first, so flip vertically (negative height)
	batch.draw_raw(color_tex, size,
		Rect{0.0f, float(size.y), float(size.x), -float(size.y)},
		Rect{offset.x, offset.y, scaled.x, scaled.y},
		glm::u8vec4(0xff));
	batch.end();
}
