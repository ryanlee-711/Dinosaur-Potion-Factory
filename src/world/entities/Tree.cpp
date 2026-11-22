#include "Tree.hpp"
#include "../World.hpp"
#include "../TileMap.hpp"
#include "../ItemEntity.hpp"
#include "../../rendering/Particles.hpp"
#include "../../starter/Load.hpp"
#include "../../starter/data_path.hpp"

#include <algorithm>
#include <cmath>
#include <random>
#include <sstream>
#include <stdexcept>

static Load< Texture > tree_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/test/tree.png")); //placeholder sprite
});

static std::mt19937 lumber_rng(0x6c6f6773); // "logs"

glm::vec2 Tree::base() const {
	return position + glm::vec2(GameConfig::TILE_SIZE, GameConfig::TILE_SIZE);
}

bool Tree::mine(int power, glm::vec2 from, std::vector<ItemRecord> &drops) {
	if (state == Stump) return false;   //nothing to chop until it regrows
	if (state == Falling) return true;  //already coming down, ignore extra swings
	hits += std::max(1, power);
	shake_timer = SHAKE_SECONDS;
	chips_pending = true;
	if (hits >= HITS_TO_FELL) {
		state = Falling;
		timer = 0.0f;
		fall_dir = (from.x <= base().x) ? 1 : -1; //fall away from whoever chopped it
	}
	return true; //lumber drops when it lands
}

void Tree::update(FrameInput const &input, float elapsed, World const &world) {
	shake_timer = std::max(0.0f, shake_timer - elapsed);

	if (chips_pending) {
		chips_pending = false;
		if (world.particles) world.particles->burst(Presets::WoodChips, base() - glm::vec2(0.0f, 8.0f));
	}

	if (state == Falling) {
		timer += elapsed;
		if (timer < FALL_SECONDS) return;

		// landed
		state = Stump;
		timer = REGROW_SECONDS;
		hits = 0;
		glm::vec2 crown = base() + glm::vec2(float(fall_dir) * 40.0f, -4.0f);
		if (world.particles) world.particles->burst(Presets::Leaves, crown);
		if (world.spawned == nullptr) return;
		std::uniform_real_distribution< float > jitter(-4.0f, 4.0f);
		for (int i = 0; i < LUMBER_DROPS; ++i) {
			glm::vec2 spot = base() + glm::vec2(float(fall_dir) * (18.0f + 12.0f * float(i)) + jitter(lumber_rng), jitter(lumber_rng));
			if (world.map.is_solid(Tilemap::tile_at_point(spot))) spot = base() + glm::vec2(0.0f, 10.0f); //landed in a wall: drop at the stump
			auto lumber = std::make_unique< ItemEntity >("lumber", 1);
			lumber->position = spot;
			world.spawned->push_back(std::move(lumber));
		}
		return;
	}

	if (state == Stump) {
		timer -= elapsed;
		if (timer <= 0.0f) {
			state = Standing;
			timer = 0.0f;
		}
	}
}

void Tree::draw(Sprites &batch) const {
	int column = 0;
	if (state == Falling) {
		column = 1 + std::min(2, int(timer / FALL_SECONDS * 3.0f));
	} else if (state == Stump) {
		column = 4;
	}
	int row = (fall_dir < 0) ? 1 : 0;
	Rect src{FRAME_W * float(column), FRAME_H * float(row), FRAME_W, FRAME_H};

	glm::vec2 top_left = glm::round(base()) - glm::vec2(PIVOT_X, PIVOT_Y);
	if (shake_timer > 0.0f) top_left.x += (std::fmod(shake_timer, 0.06f) < 0.03f) ? 1.0f : -1.0f; //wobble
	batch.draw(*tree_texture, src, top_left);
}

auto Tree::hitbox() const -> Rect {
	return Rect{position.x, position.y, 32.0f, 16.0f};
}

void Tree::get_occupied(std::vector<glm::ivec2> *ans) const {
	auto tile = Tilemap::tile_at_point(position + glm::vec2{GameConfig::TILE_SIZE / 2.0f});
	ans->emplace_back(tile);
	ans->emplace_back(tile + glm::ivec2{1, 0});
}

std::string Tree::save() const {
	std::ostringstream ss;
	ss << uid << ' ' << "tree" << ' ' << position.x << ' ' << position.y
	   << ' ' << int(state) << ' ' << hits << ' ' << fall_dir << ' ' << timer;
	return ss.str();
}

void Tree::load(std::string line) {
	std::istringstream ss(line);
	uint32_t read_uid;
	std::string entity_type;
	if (!(ss >> read_uid >> entity_type)) {
		throw std::runtime_error{"UID read failure"};
	}
	if (read_uid != uid && uid != 0) {
		throw std::runtime_error{"Loaded data mismatches the entity's UID!"};
	}
	if (!(ss >> position.x >> position.y)) {
		throw std::runtime_error{"Didn't read position"};
	}
	// the rest is optional, so a level file can just say "<uid> tree <x> <y>" for a full-grown tree
	int read_state = 0;
	if (ss >> read_state >> hits >> fall_dir >> timer) {
		if (read_state < 0 || read_state > 2) throw std::runtime_error{"Invalid tree state"};
		state = State(read_state);
	} else {
		state = Standing; hits = 0; fall_dir = 1; timer = 0.0f;
	}
}