#include "Sprites.hpp"

#include "../starter/LitColorTextureProgram.hpp"
#include "../starter/DrawLines.hpp"
#include "../starter/Mesh.hpp"
#include "../starter/Load.hpp"
#include "../starter/gl_errors.hpp"
#include "../starter/data_path.hpp"
#include "../starter/load_save_png.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <unordered_map>
#include <list>

GLuint plane_program = 0;
Load< MeshBuffer > plane_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("plane.pnct"));
	plane_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

SharedTexture::SharedTexture() {
	glGenTextures(1, &tex);
}

SharedTexture::SharedTexture(std::string file_path) {
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);

	// get PNG!
	std::vector<glm::u8vec4> image(0);
	
	load_png(file_path, &image_size, &image,  LowerLeftOrigin);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, image_size.x, image_size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);
}

SharedTexture::SharedTexture(std::vector<glm::u8vec4> image, GLsizei w, GLsizei h) {
	image_size.x = w;
	image_size.y = h;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);
}

 void SharedTexture::overwrite(std::vector<glm::u8vec4> image, GLsizei w, GLsizei h) {
	image_size.x = w;
	image_size.y = h;
	glBindTexture(GL_TEXTURE_2D, tex);
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);
}

SharedTexture::~SharedTexture() {
	if (tex != 0) {
		glDeleteTextures(1, &tex);
	}
}

Sprite2D::Sprite2D(Scene &scene): scene(scene) {
	add_drawable_and_transform();
	is_valid = true;
}

void Sprite2D::add_drawable_and_transform() {
	// initialize with scene, with texture
	// load mesh and add as drawable, and read sprite file
	static int sprite_count = 0;
	Mesh const &mesh = plane_meshes->lookup("Plane");

	// add a new transform
	scene.transforms.emplace_back();
	Scene::Transform &xform = scene.transforms.back();

	// Complete the transform and drawable init
	xform.name = "Sprite_" + std::to_string(sprite_count++);
	xform.parent = nullptr;
	xform.position = glm::vec3(0.0f);
	xform.scale = glm::vec3(1.0f, 1.0f, 1.0f);
	xform.rotation = glm::quat(glm::vec3{0.0f, 0.0f, 0.0f});

	scene.drawables.emplace_back(&xform);
	Scene::Drawable &drawable_ = scene.drawables.back();
	drawable = &drawable_;
	drawable_.pipeline = lit_color_texture_program_pipeline;
	drawable_.pipeline.vao = plane_program;
	drawable_.pipeline.type = mesh.type;
	drawable_.pipeline.start = mesh.start;
	drawable_.pipeline.count = mesh.count;
	drawable_.blended = true;

	// transform and drawable
	transform_iter = --scene.transforms.end();
	drawable_iter = --scene.drawables.end();
}

Sprite2D::Sprite2D(Scene &scene, std::string file_path): scene(scene) {
	add_drawable_and_transform();
	is_valid = true;

	// load texture from PNG file
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);

	// get PNG!
	glm::uvec2 image_size;
	std::vector<glm::u8vec4> image(0);
	
	load_png(file_path, &image_size, &image,  LowerLeftOrigin);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, image_size.x, image_size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);

	drawable->pipeline.textures[0].texture = tex;
	drawable->pipeline.textures[0].target = GL_TEXTURE_2D;

	// Set uniforms to modulate color
	drawable->pipeline.set_uniforms = [this]() {
		glUniform4fv(lit_color_texture_program->TINT_vec4, 1, glm::value_ptr(this->tint));
		glUniform3f(lit_color_texture_program->LIGHT_DIRECTION_vec3, 0.0f, 0.0f, 0.0f);
	};

	tint = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	is_valid = true;
}

Sprite2D::Sprite2D(Scene &scene, SharedTexture const &shared_texture): scene(scene) {
	add_drawable_and_transform();
	is_valid = true;

	tex = shared_texture.tex;

	drawable->pipeline.textures[0].texture = tex;
	drawable->pipeline.textures[0].target = GL_TEXTURE_2D;

	// Set uniforms to modulate color
	drawable->pipeline.set_uniforms = [this]() {
		glUniform4fv(lit_color_texture_program->TINT_vec4, 1, glm::value_ptr(this->tint));
		glUniform3f(lit_color_texture_program->LIGHT_DIRECTION_vec3, 0.0f, 0.0f, 0.0f);
	};

	tint = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	is_texture_shared = true;
}

Sprite2D::~Sprite2D() {
	if (!is_valid) return;
	// release texture...
	if (tex != 0 && !is_texture_shared) {
		glDeleteTextures(1, &tex);
	}
	scene.drawables.erase(drawable_iter);
	scene.transforms.erase(transform_iter);

}

void Sprite2D::set_position(glm::vec2 new_position) {
	position = new_position;
	drawable->transform->position.x = position.x;
	drawable->transform->position.y = position.y;
}

void Sprite2D::set_scale(glm::vec2 new_scale) {
	scale = new_scale;
	drawable->transform->scale.x = scale.x;
	drawable->transform->scale.y = scale.y;
}

void Sprite2D::set_rotation(glm::f32 new_rotation) {
	rotation = new_rotation;
	drawable->transform->rotation = glm::quat(glm::vec3(0.0f, 0.0f, rotation));
}

void Sprite2D::set_z(glm::f32 new_z) {
	z = new_z;
	drawable->transform->position.z = z;
}