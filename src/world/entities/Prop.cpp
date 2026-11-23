#include "Prop.hpp"
#include "../TileMap.hpp"
#include "../../starter/data_path.hpp"

#include <sstream>
#include <stdexcept>

static Load< Texture > woodpile_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/test/woodpile.png")); //placeholder sprite
});
static Load< Texture > chopping_stump_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/test/chopping_stump.png")); //placeholder sprite
});

// ---- PROP TYPES ----
//   type              tiles              sprite size            sprite x  solid  texture
static PropDef const prop_defs[] = {
	{"woodpile",       glm::ivec2{3, 1}, glm::vec2{48.0f, 32.0f},  0.0f,   true,  &woodpile_texture},
	{"chopping_stump", glm::ivec2{1, 1}, glm::vec2{24.0f, 32.0f}, -4.0f,   true,  &chopping_stump_texture},
};

PropDef const *PropDef::get(std::string const &type) {
	for (PropDef const &def : prop_defs) {
		if (def.type == type) return &def;
	}
	return nullptr;
}

Prop::Prop(std::string const &type) {
	set_def(type);
}

void Prop::set_def(std::string const &type) {
	def = PropDef::get(type);
	if (def == nullptr) throw std::runtime_error{"Unknown prop type: " + type};
	collision_mask = def->solid ? 0x1 : 0x0;
}

void Prop::draw(Sprites &batch) const {
	float footprint_h = float(def->tiles.y * GameConfig::TILE_SIZE);
	glm::vec2 top_left = glm::round(position) + glm::vec2(def->sprite_x, footprint_h - def->sprite_size.y);
	batch.draw(**def->texture, Rect{0.0f, 0.0f, def->sprite_size.x, def->sprite_size.y}, top_left);
}

auto Prop::hitbox() const -> Rect {
	return Rect{position.x, position.y,
		float(def->tiles.x * GameConfig::TILE_SIZE), float(def->tiles.y * GameConfig::TILE_SIZE)};
}

void Prop::get_occupied(std::vector<glm::ivec2> *ans) const {
	auto tile = Tilemap::tile_at_point(position + glm::vec2{GameConfig::TILE_SIZE / 2.0f});
	for (int dx = 0; dx < def->tiles.x; ++dx) {
		for (int dy = 0; dy < def->tiles.y; ++dy) {
			ans->emplace_back(tile + glm::ivec2{dx, dy});
		}
	}
}

std::string Prop::save() const {
	std::ostringstream ss;
	ss << uid << ' ' << def->type << ' ' << position.x << ' ' << position.y;
	return ss.str();
}

void Prop::load(std::string line) {
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
	if (!(ss >> position.x >> position.y)) {
		throw std::runtime_error{"Didn't read position"};
	}
}