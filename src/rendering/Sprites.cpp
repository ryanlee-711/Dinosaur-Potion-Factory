#include "Sprites.hpp"

#include "../starter/ColorTextureProgram.hpp"
#include "../starter/gl_errors.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <cstddef>
#include <stdexcept>

Sprites::Sprites() {
	vertices.reserve(6 * 1024);

	glGenBuffers(1, &vertex_buffer);
	glGenVertexArrays(1, &vertex_array);

	//describe the Vertex layout to the shader (ColorTextureProgram):
	glBindVertexArray(vertex_array);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

	glVertexAttribPointer(color_texture_program->Position_vec4, 2, GL_FLOAT, GL_FALSE,
		sizeof(Vertex), (GLvoid *)offsetof(Vertex, position));
	glEnableVertexAttribArray(color_texture_program->Position_vec4);

	glVertexAttribPointer(color_texture_program->Color_vec4, 4, GL_UNSIGNED_BYTE, GL_TRUE,
		sizeof(Vertex), (GLvoid *)offsetof(Vertex, color));
	glEnableVertexAttribArray(color_texture_program->Color_vec4);

	glVertexAttribPointer(color_texture_program->TexCoord_vec2, 2, GL_FLOAT, GL_FALSE,
		sizeof(Vertex), (GLvoid *)offsetof(Vertex, tex_coord));
	glEnableVertexAttribArray(color_texture_program->TexCoord_vec2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	//1x1 white texture so fill_rect can go through the same shader:
	glm::u8vec4 white(0xff);
	glGenTextures(1, &white_tex);
	glBindTexture(GL_TEXTURE_2D, white_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &white);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);

	GL_ERRORS();
}

Sprites::~Sprites() {
	glDeleteTextures(1, &white_tex);
	glDeleteVertexArrays(1, &vertex_array);
	glDeleteBuffers(1, &vertex_buffer);
}

void Sprites::begin(glm::mat4 const &world_to_clip_) {
	if (in_batch) throw std::runtime_error("Sprites::begin() called twice without end().");
	in_batch = true;
	world_to_clip = world_to_clip_;
	vertices.clear();
	current_tex = 0;
}

void Sprites::draw(Texture const &texture, Rect src, Rect dst, glm::u8vec4 tint) {
	draw_raw(texture.tex, texture.size, src, dst, tint);
}

void Sprites::draw(Texture const &texture, Rect src, glm::vec2 position, glm::u8vec4 tint) {
	draw_raw(texture.tex, texture.size, src, Rect{position.x, position.y, src.w, src.h}, tint);
}

void Sprites::fill_rect(Rect dst, glm::u8vec4 color) {
	draw_raw(white_tex, glm::uvec2(1), Rect{0.0f, 0.0f, 1.0f, 1.0f}, dst, color);
}

void Sprites::draw_raw(GLuint tex, glm::uvec2 tex_size, Rect src, Rect dst, glm::u8vec4 tint) {
	if (!in_batch) throw std::runtime_error("Sprites::draw() called outside begin()/end().");

	//switching textures means the quads so far must be drawn first:
	if (tex != current_tex) {
		flush();
		current_tex = tex;
	}

	glm::vec2 uv_min = glm::vec2(src.x, src.y) / glm::vec2(tex_size);
	glm::vec2 uv_max = glm::vec2(src.x + src.w, src.y + src.h) / glm::vec2(tex_size);

	glm::vec2 p_min(dst.x, dst.y);
	glm::vec2 p_max(dst.x + dst.w, dst.y + dst.h);

	Vertex top_left    { glm::vec2(p_min.x, p_min.y), tint, glm::vec2(uv_min.x, uv_min.y) };
	Vertex top_right   { glm::vec2(p_max.x, p_min.y), tint, glm::vec2(uv_max.x, uv_min.y) };
	Vertex bottom_left { glm::vec2(p_min.x, p_max.y), tint, glm::vec2(uv_min.x, uv_max.y) };
	Vertex bottom_right{ glm::vec2(p_max.x, p_max.y), tint, glm::vec2(uv_max.x, uv_max.y) };

	//two triangles per quad:
	vertices.emplace_back(top_left);
	vertices.emplace_back(bottom_left);
	vertices.emplace_back(bottom_right);
	vertices.emplace_back(top_left);
	vertices.emplace_back(bottom_right);
	vertices.emplace_back(top_right);
}

void Sprites::end() {
	if (!in_batch) throw std::runtime_error("Sprites::end() called without begin().");
	flush();
	in_batch = false;
}

void Sprites::flush() {
	if (vertices.empty()) return;

	//upload this frame's vertices:
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STREAM_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//2D drawing: no depth test, standard alpha blending:
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUseProgram(color_texture_program->program);
	glUniformMatrix4fv(color_texture_program->OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(world_to_clip));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, current_tex);

	glBindVertexArray(vertex_array);
	glDrawArrays(GL_TRIANGLES, 0, GLsizei(vertices.size()));
	glBindVertexArray(0);

	glBindTexture(GL_TEXTURE_2D, 0);
	glUseProgram(0);

	GL_ERRORS();

	vertices.clear();
}
