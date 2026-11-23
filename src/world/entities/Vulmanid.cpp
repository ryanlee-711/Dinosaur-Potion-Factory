#include "Vulmanid.hpp"
#include "../World.hpp"
#include "../TileMap.hpp"
#include "../../inventory/Inventory.hpp"
#include "../../rendering/Particles.hpp"
#include "../../starter/Load.hpp"
#include "../../starter/data_path.hpp"

#include <algorithm>
#include <cmath>
#include <random>
#include <sstream>
#include <stdexcept>

static Load< Texture > vulmanid_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/test/vulmanid.png")); //placeholder sprite
});

static std::mt19937 vulmanid_rng(0x76756c6d); // "vulm"

static float random_between(float lo, float hi) {
	return std::uniform_real_distribution< float >(lo, hi)(vulmanid_rng);
}

// ---------------- helpers ----------------

Rect Vulmanid::zone_rect() const {
	glm::vec2 lo = Tilemap::tile_topleft(zone_min);
	glm::vec2 hi = Tilemap::tile_topleft(zone_max + glm::ivec2(1));
	return Rect{lo.x, lo.y, hi.x - lo.x, hi.y - lo.y};
}

Rect Vulmanid::hitbox_at(glm::vec2 feet) const {
	return Rect{feet.x - HITBOX_W * 0.5f, feet.y - HITBOX_H, HITBOX_W, HITBOX_H};
}

bool Vulmanid::can_stand(glm::vec2 feet, World const &world) const {
	Rect box = hitbox_at(feet);
	Rect zone = zone_rect();
	if (box.x < zone.x || box.y < zone.y || box.x + box.w > zone.x + zone.w || box.y + box.h > zone.y + zone.h) {
		return false; //would leave its zone
	}
	//solid tiles, and solid things (trees, boulders, machines...) on the tiles it would touch.
	//(World::blocked only counts things that block the mover, and the vulmanid blocks nothing so you can run through it.)
	glm::ivec2 first = Tilemap::tile_at_point(box.min());
	glm::ivec2 last = Tilemap::tile_at_point(box.max() - glm::vec2(0.001f));
	for (int ty = first.y; ty <= last.y; ++ty) {
		for (int tx = first.x; tx <= last.x; ++tx) {
			glm::ivec2 tile(tx, ty);
			if (world.map.is_solid(tile)) return false;
			Entity *other = world.entity_at(tile);
			if (other != nullptr && other != this && (other->collision_mask & 0x1) && !other->removed) {
				Rect r = other->hitbox();
				bool overlaps = box.x < r.x + r.w && r.x < box.x + box.w && box.y < r.y + r.h && r.y < box.y + box.h;
				if (overlaps) return false;
			}
		}
	}
	return true;
}

bool Vulmanid::move(glm::vec2 step, World const &world) {
	//try the whole step, else each axis on its own (slides along walls and the zone border)
	glm::vec2 start = position;
	if (can_stand(position + step, world)) {
		position += step;
	} else {
		if (step.x != 0.0f && can_stand(position + glm::vec2(step.x, 0.0f), world)) position.x += step.x;
		if (step.y != 0.0f && can_stand(position + glm::vec2(0.0f, step.y), world)) position.y += step.y;
	}
	glm::vec2 moved = position - start;
	float distance = glm::length(moved);
	moving = distance > 0.0001f;
	if (moving) {
		step_distance += distance;
		//turn the sprite around only when it's mostly moving sideways, and steadily in the new direction
		//(sliding up/down along an edge with tiny sideways wobbles keeps the facing)
		bool sideways = std::abs(moved.x) >= 0.5f * std::abs(moved.y);
		bool against_facing = facing_left ? (moved.x > 0.0f) : (moved.x < 0.0f);
		if (sideways && against_facing) {
			turn_around += std::abs(moved.x);
			if (turn_around >= TURN_AROUND_DISTANCE) {
				facing_left = !facing_left;
				turn_around = 0.0f;
			}
		} else if (!against_facing) {
			turn_around = 0.0f;
		}
	}
	return moving;
}

void Vulmanid::start_fleeing() {
	if (state != Flee) {
		alarm_timer = 0.6f; //"!"
		flee_dir = glm::vec2(0.0f);
	}
	state = Flee;
}

// ---------------- update ----------------

void Vulmanid::update(FrameInput const &input, float elapsed, World const &world) {
	alarm_timer = std::max(0.0f, alarm_timer - elapsed);
	moving = false;

	//feathers grow back slowly
	if (feathers < MAX_FEATHERS) {
		regrow_timer -= elapsed;
		if (regrow_timer <= 0.0f) {
			regrow_timer = FEATHER_REGROW_SECONDS;
			feathers += 1;
		}
	} else {
		regrow_timer = FEATHER_REGROW_SECONDS;
	}

	if (plucked_pending) {
		plucked_pending = false;
		if (world.particles) world.particles->burst(Presets::Feathers, position - glm::vec2(0.0f, 8.0f));
	}

	glm::vec2 away = position - world.player.position;
	float player_distance = glm::length(away);

	//notice the dwarf / calm down
	if (player_distance < NOTICE_RADIUS) {
		start_fleeing();
	} else if (state == Flee && player_distance > CALM_RADIUS) {
		state = Idle;
		state_timer = random_between(0.8f, 2.0f);
	}

	if (state == Flee) {
		//pick a direction: straight away from the dwarf if that's open; else keep the direction it's already
		//running if that still works (no wobbling along edges); else turn a bit more each try (same side as last time first)
		float base = (player_distance > 0.001f) ? std::atan2(away.y, away.x) : random_between(-3.14159f, 3.14159f);
		constexpr float LOOKAHEAD = 10.0f;
		constexpr float TURN_STEP = 15.0f;
		glm::vec2 straight(std::cos(base), std::sin(base));
		bool found = false;
		glm::vec2 dir(0.0f);
		if (can_stand(position + straight * LOOKAHEAD, world)) {
			dir = straight;
			found = true;
		} else if (flee_dir != glm::vec2(0.0f)
			&& glm::dot(flee_dir, straight) >= std::cos(glm::radians(MAX_FLEE_TURN))
			&& can_stand(position + flee_dir * LOOKAHEAD, world)) {
			dir = flee_dir;
			found = true;
		}
		for (float turn = TURN_STEP; turn <= MAX_FLEE_TURN && !found; turn += TURN_STEP) {
			for (int side : {turn_side, -turn_side}) {
				float angle = base + float(side) * glm::radians(turn);
				glm::vec2 d(std::cos(angle), std::sin(angle));
				if (can_stand(position + d * LOOKAHEAD, world)) {
					dir = d;
					found = true;
					if (turn > 0.0f) turn_side = side;
					break;
				}
			}
		}
		flee_dir = found ? dir : glm::vec2(0.0f);
		if (found) move(dir * FLEE_SPEED * elapsed, world);
		//else: cornered, it just stands there
		return;
	}

	if (state == Idle) {
		state_timer -= elapsed;
		if (state_timer > 0.0f) return;
		//wander to a random spot in the zone, not too far away
		Rect zone = zone_rect();
		glm::vec2 target = position + glm::vec2(random_between(-64.0f, 64.0f), random_between(-64.0f, 64.0f));
		target.x = std::clamp(target.x, zone.x + HITBOX_W, zone.x + zone.w - HITBOX_W);
		target.y = std::clamp(target.y, zone.y + HITBOX_H + 1.0f, zone.y + zone.h - 1.0f);
		wander_target = target;
		state = Wander;
		return;
	}

	if (state == Wander) {
		glm::vec2 to_target = wander_target - position;
		float distance = glm::length(to_target);
		float step = WANDER_SPEED * elapsed;
		bool arrived = distance <= step;
		bool stuck = !arrived && !move(to_target / distance * step, world);
		if (arrived) {
			if (can_stand(wander_target, world)) position = wander_target;
		}
		if (arrived || stuck) {
			state = Idle;
			state_timer = random_between(1.0f, 3.5f);
		}
	}
}

// ---------------- interaction ----------------

void Vulmanid::on_interact(Context &ctx) {
	if (feathers <= 0) {
		ctx.message = "It has no feathers left to pluck";
		return;
	}
	if (ctx.player_inventory.add("feather", 1) > 0) {
		ctx.message = "Your bag is full";
		return;
	}
	feathers -= 1;
	plucked_pending = true;
	start_fleeing(); //and off it goes
}

// ---------------- drawing ----------------

void Vulmanid::draw(Sprites &batch) const {
	int column = moving ? 1 + int(step_distance / 6.0f) % 2 : 0;
	int row = facing_left ? 1 : 0;
	glm::vec2 top_left = glm::round(position) - glm::vec2(8.0f, 16.0f);
	batch.draw(*vulmanid_texture, Rect{16.0f * float(column), 16.0f * float(row), 16.0f, 16.0f}, top_left);
	if (alarm_timer > 0.0f) {
		batch.draw(*vulmanid_texture, Rect{48.0f, 0.0f, 16.0f, 16.0f}, top_left - glm::vec2(0.0f, 13.0f)); //"!"
	}
}

auto Vulmanid::hitbox() const -> Rect {
	return hitbox_at(position);
}

void Vulmanid::get_occupied(std::vector<glm::ivec2> *ans) const {
	ans->emplace_back(Tilemap::tile_at_point(position - glm::vec2(0.0f, 1.0f)));
}

// ---------------- save / load ----------------

std::string Vulmanid::save() const {
	std::ostringstream ss;
	ss << uid << ' ' << "vulmanid" << ' ' << position.x << ' ' << position.y
	   << ' ' << zone_min.x << ' ' << zone_min.y << ' ' << zone_max.x << ' ' << zone_max.y << ' ' << feathers;
	return ss.str();
}

void Vulmanid::load(std::string line) {
	std::istringstream ss(line);
	uint32_t read_uid;
	std::string entity_type;
	if (!(ss >> read_uid >> entity_type)) {
		throw std::runtime_error{"UID read failure"};
	}
	if (read_uid != uid && uid != 0) {
		throw std::runtime_error{"Loaded data mismatches the entity's UID!"};
	}
	if (!(ss >> position.x >> position.y >> zone_min.x >> zone_min.y >> zone_max.x >> zone_max.y)) {
		throw std::runtime_error{"Didn't read vulmanid position / zone"};
	}
	if (!(ss >> feathers)) feathers = MAX_FEATHERS; //optional in the level file
	state = Idle;
	state_timer = random_between(0.5f, 2.0f);
}