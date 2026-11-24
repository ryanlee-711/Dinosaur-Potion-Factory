#include "Creature.hpp"
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
static Load< Texture > glaber_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/test/glaber.png")); //placeholder sprite
});
static Load< Texture > ice_lizard_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/test/ice_lizard.png")); //placeholder sprite
});

// ---- CREATURE TYPES ----
//     type       drop   max drops  regrow        empty msg            wander speed   flee speed
//  notice rad   calm rad      hitbox               texture           harvest particles
static CreatureDef const creature_defs[] = {
	{"vulmanid", "feather",  2,  20.0f, "It has no feathers left to pluck", 28.0f,  PLAYER_SPRINT_SPEED,
		56.0f,   112.0f,   glm::vec2(12.0f, 8.0f), &vulmanid_texture, &Presets::Feathers,
	},
	{"glaber", "glaber_claw", 2, 20.0f, "Its claws haven't grown back yet", 22.0f, 0.8f * PLAYER_SPRINT_SPEED,
        48.0f,   96.0f,    glm::vec2(12.0f, 8.0f), &glaber_texture,   &Presets::Dirt,
	},
	{"ice_lizard", "ice_scale", 1, 30.0f, "Its scales haven't regrown yet", 30.0f, 1.3f * PLAYER_SPRINT_SPEED,
        65.0f,   140.0f,   glm::vec2(20.0f, 10.0f), &ice_lizard_texture, &Presets::Frost, glm::vec2(32.0f, 24.0f),
	},
};

CreatureDef const *CreatureDef::get(std::string const &type) {
	for (CreatureDef const &def : creature_defs) {
		if (def.type == type) return &def;
	}
	return nullptr;
}

static std::mt19937 creature_rng(0x63726561); // "crea"

static float random_between(float lo, float hi) {
	return std::uniform_real_distribution< float >(lo, hi)(creature_rng);
}

Creature::Creature(std::string const &type) {
	collision_mask = 0x0; //you can run through it
	set_def(type);
	drops = def->max_drops;
}

void Creature::set_def(std::string const &type) {
	def = CreatureDef::get(type);
	if (def == nullptr) throw std::runtime_error{"Unknown creature type: " + type};
	regrow_timer = def->regrow_seconds;
}

// ---------------- helpers ----------------

Rect Creature::zone_rect() const {
	glm::vec2 lo = Tilemap::tile_topleft(zone_min);
	glm::vec2 hi = Tilemap::tile_topleft(zone_max + glm::ivec2(1));
	return Rect{lo.x, lo.y, hi.x - lo.x, hi.y - lo.y};
}

Rect Creature::hitbox_at(glm::vec2 feet) const {
	return Rect{feet.x - def->hitbox_size.x * 0.5f, feet.y - def->hitbox_size.y, def->hitbox_size.x, def->hitbox_size.y};
}

bool Creature::can_stand(glm::vec2 feet, World const &world) const {
	Rect box = hitbox_at(feet);
	Rect zone = zone_rect();
	if (box.x < zone.x || box.y < zone.y || box.x + box.w > zone.x + zone.w || box.y + box.h > zone.y + zone.h) {
		return false; //would leave its zone
	}
	//solid tiles, and solid things (trees, boulders, machines...) on the tiles it would touch.
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

bool Creature::move(glm::vec2 step, World const &world) {
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

void Creature::start_fleeing() {
	if (state != Flee) {
		alarm_timer = 0.6f; //"!"
		flee_dir = glm::vec2(0.0f);
	}
	state = Flee;
}

// ---------------- update ----------------

void Creature::update(FrameInput const &input, float elapsed, World const &world) {
	alarm_timer = std::max(0.0f, alarm_timer - elapsed);
	moving = false;

	//what you take from it grows back slowly
	if (drops < def->max_drops) {
		regrow_timer -= elapsed;
		if (regrow_timer <= 0.0f) {
			regrow_timer = def->regrow_seconds;
			drops += 1;
		}
	} else {
		regrow_timer = def->regrow_seconds;
	}

	if (harvested_pending) {
		harvested_pending = false;
		if (world.particles && def->harvest_particles) world.particles->burst(*def->harvest_particles, position - glm::vec2(0.0f, 8.0f));
	}

	glm::vec2 away = position - world.player.position;
	float player_distance = glm::length(away);

	//notice the dwarf / calm down
	if (player_distance < def->notice_radius) {
		start_fleeing();
	} else if (state == Flee && player_distance > def->calm_radius) {
		state = Idle;
		state_timer = random_between(0.8f, 2.0f);
	}

	if (state == Flee) {
		//pick a direction: straight away from the dwarf if that's open, else keep the direction it's already
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
		if (found) move(dir * def->flee_speed * elapsed, world);
		//else: cornered, it just stands there
		return;
	}

	if (state == Idle) {
		state_timer -= elapsed;
		if (state_timer > 0.0f) return;
		//wander to a random spot in the zone, not too far away
		Rect zone = zone_rect();
		glm::vec2 target = position + glm::vec2(random_between(-WANDER_DISTANCE, WANDER_DISTANCE), random_between(-WANDER_DISTANCE, WANDER_DISTANCE));
		target.x = std::clamp(target.x, zone.x + def->hitbox_size.x, zone.x + zone.w - def->hitbox_size.x);
		target.y = std::clamp(target.y, zone.y + def->hitbox_size.y + 1.0f, zone.y + zone.h - 1.0f);
		wander_target = target;
		state = Wander;
		return;
	}

	if (state == Wander) {
		glm::vec2 to_target = wander_target - position;
		float distance = glm::length(to_target);
		float step = def->wander_speed * elapsed;
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

void Creature::on_interact(Context &ctx) {
	if (drops <= 0) {
		ctx.message = def->empty_message;
		return;
	}
	if (ctx.player_inventory.add(def->drop, 1) > 0) {
		ctx.message = "Your bag is full";
		return;
	}
	drops -= 1;
	ctx.picked_up_item = true;
	harvested_pending = true;
	start_fleeing(); //and off it goes
}

// ---------------- drawing ----------------

void Creature::draw(Sprites &batch) const {
	int column = moving ? 1 + int(step_distance / 6.0f) % 2 : 0;
	int row = facing_left ? 1 : 0;
	float w = def->frame_size.x, h = def->frame_size.y;
	glm::vec2 top_left = glm::round(position) - glm::vec2(std::floor(w * 0.5f), h); //bottom-center on the feet
	batch.draw(**def->texture, Rect{w * float(column), h * float(row), w, h}, top_left);
	if (alarm_timer > 0.0f) {
		batch.draw(**def->texture, Rect{3.0f * w, 0.0f, w, h}, top_left - glm::vec2(0.0f, h - 3.0f)); //"!" (column 3, row 0)
	}
}

auto Creature::hitbox() const -> Rect {
	return hitbox_at(position);
}

void Creature::get_occupied(std::vector<glm::ivec2> *ans) const {
	ans->emplace_back(Tilemap::tile_at_point(position - glm::vec2(0.0f, 1.0f)));
}

// ---------------- save / load ----------------

std::string Creature::save() const {
	std::ostringstream ss;
	ss << uid << ' ' << def->type << ' ' << position.x << ' ' << position.y
	   << ' ' << zone_min.x << ' ' << zone_min.y << ' ' << zone_max.x << ' ' << zone_max.y << ' ' << drops;
	return ss.str();
}

void Creature::load(std::string line) {
	std::istringstream ss(line);
	uint32_t read_uid;
	std::string entity_type;
	if (!(ss >> read_uid >> entity_type)) {
		throw std::runtime_error{"UID read failure"};
	}
	if (read_uid != uid && uid != 0) {
		throw std::runtime_error{"Loaded data mismatches the entity's UID!"};
	}
	set_def(entity_type);
	if (!(ss >> position.x >> position.y >> zone_min.x >> zone_min.y >> zone_max.x >> zone_max.y)) {
		throw std::runtime_error{"Didn't read creature position / zone"};
	}
	if (!(ss >> drops)) drops = def->max_drops; //optional in the level file
	state = Idle;
	state_timer = random_between(0.5f, 2.0f);
}
