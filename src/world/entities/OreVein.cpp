#include "OreVein.hpp"
#include "../World.hpp"
#include "../TileMap.hpp"
#include "../../starter/data_path.hpp"

#include <cmath>
#include <sstream>
#include <stdexcept>

static Load< Texture > copper_vein_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/sprites/entities/copper_vein.png"));
});
static Load< Texture > iron_vein_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/test/iron_vein.png")); //placeholder sprite
});
static Load< Texture > snow_gem_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/test/snowgem_vein.png")); //placeholder sprite
});

// ---- ORE VEIN TYPES ----
//   type            drop          max   regrow(s)   texture
static OreVeinDef const ore_vein_defs[] = {
	{"copper_vein", "copper_ore",  5,   6.0f,      &copper_vein_texture},
	{"iron_vein",   "iron_ore",    5,   6.0f,      &iron_vein_texture},
	{"snowgem_vein",    "snowgem",    1,  40.0f,      &snow_gem_texture},
};

OreVeinDef const *OreVeinDef::get(std::string const &type) {
	for (OreVeinDef const &def : ore_vein_defs) {
		if (def.type == type) return &def;
	}
	return nullptr;
}

OreVein::OreVein(std::string const &type) {
	def = OreVeinDef::get(type);
	if (def == nullptr) throw std::runtime_error{"Unknown ore vein type: " + type};
	ores = def->max_ores;
	regrow_timer = def->regrow_seconds;
}

void OreVein::update(FrameInput const &input, float elapsed, World const &world) {
	// regrow one ore every regrow_seconds, but only while it's not full
	if (ores >= def->max_ores) {
		regrow_timer = def->regrow_seconds;
		return;
	}
	regrow_timer -= elapsed;
	if (regrow_timer <= 0.0f) {
		regrow_timer = def->regrow_seconds;
		ores += 1;
	}
}

bool OreVein::mine(int power, glm::vec2 from, std::vector<ItemRecord> &drops) {
	if (ores <= 0) return false; //mined out: wait for it to regrow
	ores -= 1;
	drops.emplace_back(def->drop, 1);
	return true;
}

void OreVein::draw(Sprites &batch) const {
	// frame 1 = has ore, frame 0 = mined out
	Rect src{ores > 0 ? 32.0f : 0.0f, 0.0f, 32.0f, 32.0f};
	glm::vec2 top_left = glm::round(position) - glm::vec2(0.0f, 16.0f);
	batch.draw(**def->texture, src, top_left);
}

auto OreVein::hitbox() const -> Rect {
	return Rect{position.x, position.y, 32.0f, 16.0f};
}

void OreVein::get_occupied(std::vector<glm::ivec2> *ans) const {
	auto tile = Tilemap::tile_at_point(position + glm::vec2{GameConfig::TILE_SIZE / 2.0f});
	ans->emplace_back(tile);
	ans->emplace_back(tile + glm::ivec2{1, 0});
}

std::string OreVein::save() const {
	std::ostringstream ss;
	ss << uid << ' ' << def->type << ' ' << position.x << ' ' << position.y << ' ' << ores;
	return ss.str();
}

void OreVein::load(std::string line) {
	std::istringstream ss(line);
	uint32_t read_uid;
	std::string type;
	if (!(ss >> read_uid >> type)) {
		throw std::runtime_error{"UID read failure"};
	}
	if (read_uid != uid && uid != 0) {
		throw std::runtime_error{"Loaded data mismatches the entity's UID!"};
	}
	def = OreVeinDef::get(type);
	if (def == nullptr) throw std::runtime_error{"Unknown ore vein type: " + type};
	if (!(ss >> position.x >> position.y >> ores)) {
		throw std::runtime_error{"Didn't read position / ores count"};
	}
	regrow_timer = def->regrow_seconds;
}