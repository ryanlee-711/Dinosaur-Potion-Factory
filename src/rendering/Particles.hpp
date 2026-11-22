#pragma once

#include "Sprites.hpp"

#include <glm/glm.hpp>

#include <random>
#include <vector>

enum class Motion { Ballistic, Orbit };
enum class Fade { Linear, Exponential, Pulse };

struct ParticlePreset {
	Motion motion = Motion::Ballistic;
	int count_min = 1, count_max = 1;
	glm::vec2 spawn_box = glm::vec2(0.0f);
	float life_min = 1.0f, life_max = 1.0f;

	float speed_min = 0.0f, speed_max = 0.0f;
	float angle = -90.0f, spread = 360.0f;
	glm::vec2 accel = glm::vec2(0.0f);
	float drag = 0.0f;

	float radius_min = 0.0f, radius_max = 0.0f;
	float spin_min = 0.0f, spin_max = 0.0f;

	glm::u8vec4 color_start = glm::u8vec4(0xff);
	glm::u8vec4 color_end = glm::u8vec4(0xff, 0xff, 0xff, 0x00);
	Fade fade = Fade::Linear;
	int frame_first = 0, frame_count = 1;
	bool behind = false;
};

struct Particle {
	ParticlePreset const *preset;
	glm::vec2 pos, vel;
	glm::vec2 center;
	float radius, theta, spin;
	float age, life;
};

struct ParticleSystem {
	void spawn_one(ParticlePreset const &preset, glm::vec2 pos);
	void burst(ParticlePreset const &preset, glm::vec2 pos);
	void update(float elapsed);
	void draw(Sprites &batch, bool behind) const;

	std::vector< Particle > particles;
	std::mt19937 rng;
	float random(float lo, float hi);
};

struct ParticleEmitter {
	float rate = 10.0f;
	float accumulator = 0.0f;
	void tick(ParticleSystem &system, ParticlePreset const &preset, glm::vec2 pos, float elapsed);
};

namespace Presets {
	extern ParticlePreset const RockDebris;
	extern ParticlePreset const StepDust;
	extern ParticlePreset const WoodChips; //axe hits a tree
	extern ParticlePreset const Leaves;    //a tree lands on the ground
}