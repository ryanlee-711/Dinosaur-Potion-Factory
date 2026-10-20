#pragma once

#include <algorithm>
#include <list>
#include <string.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "gl_errors.hpp"
#include "read_write_chunk.hpp"

#include "Scene.hpp"

// SharedTeture!
// Sharing reused textures
struct SharedTexture {
	GLuint tex = 0;
	glm::uvec2 image_size;

	SharedTexture();
	SharedTexture(std::string file_path);
	SharedTexture(std::vector<glm::u8vec4> image, GLsizei w, GLsizei h);
	~SharedTexture();

	void overwrite(std::vector<glm::u8vec4> image, GLsizei w, GLsizei h);
};

// Sprite2D!
// Managing lifecycle of sprite objects, bound to one Scene.
// TODO: maybe move the sprites from PlayMode into Sprite2D as a singleton (static)
// TODO: z-index is not actively sorted, so there should be a callback (sorting) when anything was modified
struct Sprite2D {
	Scene::Drawable *drawable = nullptr;
	std::string name;
	glm::vec4 tint;

	bool is_valid = false;
	bool is_texture_shared = false;

	Sprite2D(Scene &scene);
	Sprite2D(Scene &scene, std::string file_path);
	Sprite2D(Scene &scene, SharedTexture const &shared_texture);
	virtual ~Sprite2D();

	glm::vec2 get_position() const { return position; };
	glm::vec2 get_scale() const { return scale; };
	glm::f32 get_rotation() const { return rotation; };
	glm::f32 get_z() const { return z; };
	virtual void set_position(glm::vec2 new_position);
	virtual void set_scale(glm::vec2 new_scale);
	virtual void set_rotation(glm::f32 new_rotation);
	virtual void set_z(glm::f32 new_z);

	// No copy! no move!
	Sprite2D(Sprite2D const &) = delete;
	Sprite2D &operator=(Sprite2D const &) = delete;
	Sprite2D(Sprite2D &) = delete;
	Sprite2D &operator=(Sprite2D &) = delete;

private:
	GLuint tex = 0;
	// order of applying: scale --> rotation --> position
	glm::vec2 position {0.0f, 0.0f};  // position offset
	glm::vec2 scale {1.0f, 1.0f};     // axis-aligned scale
	glm::f32 rotation {0.0f};   // euler angle
	glm::f32 z {0.0f}; // z index

	// helping with access
	Scene &scene;
	std::list<Scene::Drawable>::iterator drawable_iter;
	std::list<Scene::Transform>::iterator transform_iter;

	// constructor shared
	void add_drawable_and_transform();
};