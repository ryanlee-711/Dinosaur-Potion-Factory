#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"
#include "load_save_png.hpp"
#include "GameMap.hpp"
#include "Sprites.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <unordered_map>
#include <list>
#include <random>

GLuint plane_program2 = 0;
Load< MeshBuffer > plane_meshes2(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("plane.pnct"));
	plane_program2 = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});


Load< Scene > main_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("plane.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = plane_meshes2->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = plane_program2;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

		// force set invisible for existing stuff
		drawable.blended = true;
		drawable.pipeline.set_uniforms = []() {
			glUniform4fv(lit_color_texture_program->TINT_vec4, 1, glm::value_ptr(glm::vec4(0.0f)));
		};

	});
});

Load< SharedTexture > rocket_texture(LoadTagDefault, []() -> SharedTexture const * {
	return new SharedTexture(data_path("assets/rocket.png"));
});

Load< SharedTexture > asteroid_texture(LoadTagDefault, []() -> SharedTexture const * {
	return new SharedTexture(data_path("assets/asteroid.png"));
});

Load< SharedTexture > pea_texture(LoadTagDefault, []() -> SharedTexture const * {
	return new SharedTexture(data_path("assets/pea.png"));
});

Load< SharedTexture > star_yellow_texture(LoadTagDefault, []() -> SharedTexture const * {
	return new SharedTexture(data_path("assets/star_yellow.png"));
});

Load< SharedTexture > big_star_texture(LoadTagDefault, []() -> SharedTexture const * {
	return new SharedTexture(data_path("assets/star.png"));
});

Load< SharedTexture > star_blue_texture(LoadTagDefault, []() -> SharedTexture const * {
	return new SharedTexture(data_path("assets/star_blue.png"));
});

Load< SharedTexture > goal_texture(LoadTagDefault, []() -> SharedTexture const * {
	return new SharedTexture(data_path("assets/goal.png"));
});

Load< SharedTexture > flame_texture(LoadTagDefault, []() -> SharedTexture const * {
	return new SharedTexture(data_path("assets/flame.png"));
});

Load< SharedTexture > smoke_texture(LoadTagDefault, []() -> SharedTexture const * {
	return new SharedTexture(data_path("assets/smoke.png"));
});

Load< SharedTexture > congrat_texture(LoadTagDefault, []() -> SharedTexture const * {
	return new SharedTexture(data_path("assets/congrat.png"));
});

float cross(glm::vec2 a, glm::vec2 b) {
	return a.x * b.y - b.x * a.y;
}

const glm::vec2 GOAL_POS = glm::vec2(50.0f, -10.0f);

PlayMode::PlayMode() : scene(*main_scene), text_renderer("sample", data_path("assets/fonts/NotoSans.ttf")) {
	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	//construct level.
	{
		std::mt19937 mt(GameMap::LEVEL_SEED);
		for (size_t y = 0; y < GameMap::MAP_H; ++y) {
			for (size_t x = 0; x < GameMap::MAP_W; ++x) {
				if (!GameMap::LEVEL_1[y * GameMap::MAP_W + x]) continue;
				float sprsize = mt() / float(mt.max()) * 1.0f + 5.0f;
				Sprite2D *spr = add_sprite(*asteroid_texture.value, glm::vec2(0.0f), glm::vec2(sprsize));
				spr->tint.r = spr->tint.b = mt() / float(mt.max()) * 0.25f + 0.75f;
				RigidBody *rock = add_body(
					/* Rock name */ "Rock" + std::to_string(y) + "_" + std::to_string(x),
					/* Position */ glm::vec2(-60.0f +12.8f * x + 3.6f * mt() / float(mt.max()), -32.0f + 10.5f * y + 3.8f * mt() / float(mt.max())),
					/*weight*/ 10.5f + 3.0f * mt() / float(mt.max()),
					/*radius*/ sprsize * 0.75f,
					spr
				);
				rock->rotation = mt() / float(mt.max()) * PI * 2.0f;
			}
		}
	}

	// add a goal
	{
		add_sprite(*goal_texture.value, GOAL_POS, glm::vec2(5.0f));
	}

	//add one cute rocket
	{
		std::mt19937 mt(GameMap::LEVEL_SEED | 0x12b34);
		// const & brings mental damage
		Sprite2D *spr = add_sprite(*rocket_texture.value, glm::vec2(0.0f), glm::vec2(6.0f));
		rocket = add_body("Rocket", glm::vec2(-45.0f, 28.0f), 5.0f, 4.0f, spr);
	}

	//add some random decoration stars
	{
		std::mt19937 mt(GameMap::LEVEL_SEED | 0x12b34);
		for (size_t i = 0; i < 28; ++i) {
			Sprite2D *spr = add_sprite(
				*star_blue_texture.value,
				glm::vec2(-70.0f + mt() / float(mt.max()) * 130.0f, -45.0f + mt() / float(mt.max()) * 80.0f),
				glm::vec2(2.0f + glm::vec2(mt() / float(mt.max()) * 1.5f)));
			spr->tint.a = mt() / float(mt.max()) * 0.25f;
			spr->set_z(3.0f);
		}
		for (size_t i = 0; i < 11; ++i) {
			Sprite2D *spr = add_sprite(
				*star_yellow_texture.value,
				glm::vec2(-70.0f + mt() / float(mt.max()) * 130.0f, -45.0f + mt() / float(mt.max()) * 80.0f),
				glm::vec2(1.0f + glm::vec2(mt() / float(mt.max()) * 1.5f)));
			spr->tint.a = mt() / float(mt.max()) * 0.25f;
			spr->set_z(3.0f);
		}
	}

	// propulsor? flame
	flame = add_sprite(*flame_texture.value, glm::vec2(0.0f), glm::vec2(6.0f));

	// z-sort sprites
	scene.drawables.sort([](Scene::Drawable const &dr1, Scene::Drawable const &dr2) {return dr1.transform->position.z > dr2.transform->position.z;});
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_EVENT_KEY_DOWN) {
		if (evt.key.key == SDLK_ESCAPE) {
			SDL_SetWindowRelativeMouseMode(Mode::window, false);
			return true;
		} else if (evt.key.key == SDLK_A) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_D) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_W) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_S) {
			down.downs += 1;
			down.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_SPACE) {
			shoot.downs += 1;
			shoot.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_R) {
			rewind.downs += 1;
			rewind.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_1) {
			framestepping_interval = TIMESTEP;
			physics_frame_delta = 0.0f;
			return true;
		} else if (evt.key.key == SDLK_2) {
			framestepping_interval = TIMESTEP * 2;
			physics_frame_delta = 0.0f;
			return true;
		} else if (evt.key.key == SDLK_3) {
			framestepping_interval = TIMESTEP * 5;
			physics_frame_delta = 0.0f;
			return true;
		} else if (evt.key.key == SDLK_4) {
			framestepping_interval = 0.5f;
			physics_frame_delta = 0.0f;
			return true;
		}
	} else if (evt.type == SDL_EVENT_KEY_UP) {
		if (evt.key.key == SDLK_A) {
			left.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_D) {
			right.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_W) {
			up.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_S) {
			down.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_SPACE) {
			shoot.pressed = false;
			shoot.downs = 0;
			return true;
		} else if (evt.key.key == SDLK_R) {
			rewind.pressed = false;
			rewind.downs = 0;
			return true;
		} 
	}

	return false;
}

void PlayMode::update(float elapsed) {
	// --------------------------- AESTHETICS ----------------------------

	{ // update body sprites

		for (auto iter = bodies.begin(); iter != bodies.end(); ++iter) {
			iter->sprite->set_position(iter->position);
			iter->sprite->set_rotation(iter->rotation);
		}
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.002f, 0.002f, 0.002f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	GL_ERRORS(); //print any errors produced by this setup code

	scene.draw(*camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;
		std::string text = "A/D to rotate; Hold SPACE then release to fire!";
		text += " shot=" + std::to_string(bullets_shot) + ", damage=" + std::to_string(damage);

		lines.draw_text(text,
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text(text,
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	}
}

RigidBody::RigidBody() { }

RigidBody::RigidBody(glm::vec2 position, float mass, float radius, Sprite2D *sprite)
	: position(position), mass(mass), radius(radius), sprite(sprite) {
	velocity = glm::vec2(0.0f);
}

RigidBody::~RigidBody() {
	/* Limitation: it is not cleaning up Sprite2D ref. */
	velocity = glm::vec2(0.0f);
}


/* Helpers */

Sprite2D* PlayMode::add_sprite(std::string file_path, glm::vec2 position, glm::vec2 scale) {
	sprites.emplace_back(scene, data_path("assets/rocket.png"));
	sprites.back().set_position(position);
	sprites.back().set_scale(scale);
	sprites.back().set_rotation(0.0f);
	return &sprites.back();
}

Sprite2D* PlayMode::add_sprite(SharedTexture const &shared_texture, glm::vec2 position, glm::vec2 scale) {
	sprites.emplace_back(scene, shared_texture);
	sprites.back().set_position(position);
	sprites.back().set_scale(scale);
	sprites.back().set_rotation(0.0f);
	return &sprites.back();
}

RigidBody* PlayMode::add_body(std::string name, glm::vec2 position, float mass, float radius, Sprite2D *sprite) {
	bodies.emplace_back(position, mass, radius, sprite);
	bodies.back().name = name;
	return &bodies.back();
}