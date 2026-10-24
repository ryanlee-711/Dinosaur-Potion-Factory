#include "Texture.hpp"

#include "../starter/load_save_png.hpp"
#include "../starter/gl_errors.hpp"

#include <vector>

Texture::Texture(std::string const &png_path) {
	std::vector< glm::u8vec4 > pixels;
	//Row 0 of the upload is the top row, so texture coordinate v=0 is the top edge
	load_png(png_path, &size, &pixels, UpperLeftOrigin);

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

	//GL_NEAREST keeps pixels crisp instead of blurring them
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	GL_ERRORS();
}

Texture::~Texture() {
	if (tex != 0) glDeleteTextures(1, &tex);
	tex = 0;
}
