#include "Particles.hpp"
#include "Texture.hpp"
#include "../starter/Load.hpp"
#include "../starter/data_path.hpp"

#include <glm/gtc/constants.hpp>

#include <cmath>

float ParticleSystem::random(float lo, float hi) {
	return lo + (hi - lo) * std::uniform_real_distribution< float >(0.0f, 1.0f)(rng);
}

void ParticleSystem::spawn_one(ParticlePreset const &preset, glm::vec2 pos) {
	Particle p;
	p.preset = &preset;
	p.pos = pos + glm::vec2(random(-0.5f, 0.5f) * preset.spawn_box.x, random(-0.5f, 0.5f) * preset.spawn_box.y);

	float radians = glm::radians(preset.angle + random(-0.5f, 0.5f) * preset.spread);
	float speed = random(preset.speed_min, preset.speed_max);
	p.vel = speed * glm::vec2(std::cos(radians), std::sin(radians));

	p.center = p.pos;
	p.radius = random(preset.radius_min, preset.radius_max);
	p.theta = random(0.0f, 2.0f * glm::pi< float >());
	p.spin = random(preset.spin_min, preset.spin_max);

	p.age = 0.0f;
	p.life = random(preset.life_min, preset.life_max);
	particles.push_back(p);
}

void ParticleSystem::burst(ParticlePreset const &preset, glm::vec2 pos) {
	int count = std::uniform_int_distribution< int >(preset.count_min, preset.count_max)(rng);
	for (int i = 0; i < count; ++i) {
		spawn_one(preset, pos);
	}
}

void ParticleSystem::update(float elapsed) {
	for (size_t i = 0; i < particles.size(); ) {
		Particle &p = particles[i];
		p.age += elapsed;
		if (p.age >= p.life) {
			p = particles.back();
			particles.pop_back();
			continue;
		}

		if (p.preset->motion == Motion::Ballistic) {
			p.vel += p.preset->accel * elapsed;
			p.vel *= std::exp(-p.preset->drag * elapsed);
			p.pos += p.vel * elapsed;
		} else {
			p.center += p.vel * elapsed;
			p.theta += p.spin * elapsed;
			p.pos = p.center + p.radius * glm::vec2(std::cos(p.theta), std::sin(p.theta));
		}
		++i;
	}
}

void ParticleEmitter::tick(ParticleSystem &system, ParticlePreset const &preset, glm::vec2 pos, float elapsed) {
	accumulator += rate * elapsed;
	while (accumulator >= 1.0f) {
		system.spawn_one(preset, pos);
		accumulator -= 1.0f;
	}
}

static Load< Texture > particle_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/sprites/particles.png"));
});

static float fade_amount(Fade fade, float t) {
	if (fade == Fade::Linear) return t;
	if (fade == Fade::Exponential) return (std::exp(6.0f * t) - 1.0f) / (std::exp(6.0f) - 1.0f);
	return 1.0f - std::sin(glm::pi< float >() * t);
}

void ParticleSystem::draw(Sprites &batch, bool behind) const {
	float cell = 8.0f;
	int columns = int(particle_texture->size.x) / 8;
	for (Particle const &p : particles) {
		ParticlePreset const &preset = *p.preset;
		if (preset.behind != behind) continue;

		float t = p.age / p.life;
		float f = fade_amount(preset.fade, t);
		glm::u8vec4 color = glm::u8vec4(glm::mix(glm::vec4(preset.color_start), glm::vec4(preset.color_end), f));

		int frame = preset.frame_first + int(t * float(preset.frame_count));
		Rect src{ float(frame % columns) * cell, float(frame / columns) * cell, cell, cell };
		batch.draw(*particle_texture, src, glm::round(p.pos) - glm::vec2(cell * 0.5f), color);
	}
}

namespace Presets {
	ParticlePreset const RockDebris{
		.count_min = 8, .count_max = 12,
		.spawn_box = glm::vec2(10.0f, 10.0f),
		.life_min = 0.35f, .life_max = 0.6f,
		.speed_min = 25.0f, .speed_max = 60.0f,
		.angle = -90.0f, .spread = 160.0f,
		.accel = glm::vec2(0.0f, 180.0f),
		.drag = 2.0f,
		.color_start = glm::u8vec4(0x8a, 0x80, 0x78, 0xff),
		.color_end = glm::u8vec4(0x5a, 0x52, 0x4c, 0x00),
		.fade = Fade::Exponential,
	};

	ParticlePreset const StepDust{
		.count_min = 2, .count_max = 3,
		.spawn_box = glm::vec2(6.0f, 2.0f),
		.life_min = 0.3f, .life_max = 0.5f,
		.speed_min = 4.0f, .speed_max = 10.0f,
		.angle = -90.0f, .spread = 120.0f,
		.drag = 4.0f,
		.color_start = glm::u8vec4(0xc8, 0xb8, 0x98, 0xa0),
		.color_end = glm::u8vec4(0xc8, 0xb8, 0x98, 0x00),
		.fade = Fade::Linear,
		.frame_first = 1, .frame_count = 3,
		.behind = true,
	};

	ParticlePreset const WoodChips{
		.count_min = 4, .count_max = 6,
		.spawn_box = glm::vec2(8.0f, 6.0f),
		.life_min = 0.3f, .life_max = 0.5f,
		.speed_min = 25.0f, .speed_max = 55.0f,
		.angle = -90.0f, .spread = 140.0f,
		.accel = glm::vec2(0.0f, 200.0f),
		.drag = 2.0f,
		.color_start = glm::u8vec4(0xc8, 0x96, 0x5a, 0xff),
		.color_end = glm::u8vec4(0x8a, 0x5e, 0x34, 0x00),
		.fade = Fade::Exponential,
	};

	ParticlePreset const Leaves{
		.count_min = 14, .count_max = 20,
		.spawn_box = glm::vec2(28.0f, 14.0f),
		.life_min = 0.5f, .life_max = 0.9f,
		.speed_min = 15.0f, .speed_max = 45.0f,
		.angle = -90.0f, .spread = 200.0f,
		.accel = glm::vec2(0.0f, 60.0f),
		.drag = 3.0f,
		.color_start = glm::u8vec4(0x5c, 0xa8, 0x48, 0xff),
		.color_end = glm::u8vec4(0x3a, 0x78, 0x30, 0x00),
		.fade = Fade::Linear,
	};
}