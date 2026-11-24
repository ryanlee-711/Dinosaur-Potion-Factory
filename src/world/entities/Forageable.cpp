#include "Forageable.hpp"
#include "../World.hpp"
#include "../TileMap.hpp"
#include "../../inventory/Inventory.hpp"
#include "../../starter/data_path.hpp"

#include <cmath>
#include <sstream>
#include <stdexcept>

static Load< Texture > lichen_rock_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/test/lichen_rock.png"));
});
static Load< Texture > fae_grass_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/test/fae_grass.png")); //placeholder sprite
});
static Load< Texture > fire_flower_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/test/flameflower.png")); //placeholder sprite
});

// ---- FORAGEABLE TYPES ----
//   type            drop     max   regrow(s)  tiles         sprite size texture      solid  empty message      texture
static ForageableDef const forageable_defs[] = {
	{"lichen_rock", "lichen",  5,   5.0f,  glm::ivec2{2, 1},  glm::vec2{32.0f, 32.0f}, true,  "No lichen yet", &lichen_rock_texture},
	{"fae_grass", "faegrass", 3,   8.0f,  glm::ivec2{1, 1},  glm::vec2{16.0f, 32.0f}, false, "No fae grass yet", &fae_grass_texture},
	{"flameflower", "flower", 2, 15.0f, glm::ivec2{1, 1}, glm::vec2{16.0f, 32.0f}, false, "Flame flower hasn't rekindled yet", &fire_flower_texture},
};

ForageableDef const *ForageableDef::get(std::string const &type) {
	for (ForageableDef const &def : forageable_defs) {
		if (def.type == type) return &def;
	}
	return nullptr;
}

Forageable::Forageable(std::string const &type) {
	set_def(type);
	count = def->max_count;
}

void Forageable::set_def(std::string const &type) {
	def = ForageableDef::get(type);
	if (def == nullptr) throw std::runtime_error{"Unknown forageable type: " + type};
	collision_mask = def->solid ? 0x1 : 0x0;
	regrow_timer = def->regrow_seconds;
}

void Forageable::update(FrameInput const &input, float elapsed, World const &world) {
	// regrow one every regrow_seconds, but only while it's not full
	if (count >= def->max_count) {
		regrow_timer = def->regrow_seconds;
		return;
	}
	regrow_timer -= elapsed;
	if (regrow_timer <= 0.0f) {
		regrow_timer = def->regrow_seconds;
		count += 1;
	}
}

void Forageable::on_interact(Context &ctx) {
	if (count <= 0) {
		ctx.message = def->empty_message;
		return;
	}
	// pick everything that fits in the bag; whatever doesn't fit stays on it
	int left = ctx.player_inventory.add(def->drop, count);
	ctx.picked_up_item = left < count;
	count = left;
	if (left > 0) ctx.message = "Your bag is full";
}

void Forageable::draw(Sprites &batch) const {
	// frame 1 = ready to pick, frame 0 = picked bare
	float w = def->sprite_size.x, h = def->sprite_size.y;
	Rect src{count > 0 ? w : 0.0f, 0.0f, w, h};
	float footprint_h = float(def->tiles.y * GameConfig::TILE_SIZE);
	glm::vec2 top_left = glm::round(position) + glm::vec2(0.0f, footprint_h - h);
	batch.draw(**def->texture, src, top_left);
}

glm::vec2 Forageable::hint_anchor() const {
	//centered over the footprint, just above the top of the sprite
	float footprint_h = float(def->tiles.y * GameConfig::TILE_SIZE);
	return position + glm::vec2(float(def->tiles.x * GameConfig::TILE_SIZE) * 0.5f, footprint_h - def->sprite_size.y - 1.0f);
}

auto Forageable::hitbox() const -> Rect {
	return Rect{position.x, position.y,
		float(def->tiles.x * GameConfig::TILE_SIZE), float(def->tiles.y * GameConfig::TILE_SIZE)};
}

void Forageable::get_occupied(std::vector<glm::ivec2> *ans) const {
	auto tile = Tilemap::tile_at_point(position + glm::vec2{GameConfig::TILE_SIZE / 2.0f});
	for (int dx = 0; dx < def->tiles.x; ++dx) {
		for (int dy = 0; dy < def->tiles.y; ++dy) {
			ans->emplace_back(tile + glm::ivec2{dx, dy});
		}
	}
}

std::string Forageable::save() const {
	std::ostringstream ss;
	ss << uid << ' ' << def->type << ' ' << position.x << ' ' << position.y << ' ' << count;
	return ss.str();
}

void Forageable::load(std::string line) {
	std::istringstream ss(line);
	uint32_t read_uid;
	std::string type;
	if (!(ss >> read_uid >> type)) {
		throw std::runtime_error{"UID read failure"};
	}
	if (read_uid != uid && uid != 0) {
		throw std::runtime_error{"Loaded data mismatches the entity's UID!"};
	}
	set_def(type);
	if (!(ss >> position.x >> position.y >> count)) {
		throw std::runtime_error{"Didn't read position / count"};
	}
}
