#include "Npc.hpp"
#include "../World.hpp"
#include "../TileMap.hpp"
#include "../../starter/data_path.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>

static Load< Texture > lumberjack_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/test/lumberjack.png")); //placeholder sprite
});
static Load< Texture > ice_elf_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/test/ice_elf.png")); //placeholder sprite
});

static Load< Texture > woodbridge_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/sprites/bridge.png")); // the bridge
});

static Load< Texture > waystone_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/test/waystone.png")); // the waysonte
});


// ---- NPC TYPES ----
//   type          dialogue      side quest         trade in                  trade out         after quest  seconds   texture              frame size
static NpcDef const npc_defs[] = {
	{"lumberjack", "lumberjack", "lumberjack_axe", ItemRecord{"lumber", 1}, ItemRecord{"planks", 2}, true,   3.0f, &lumberjack_texture, glm::vec2{32.0f, 32.0f}},
	{"ice_elf",    "ice_elf",    "ice_elf_lore",   ItemRecord{"", 0},       ItemRecord{"", 0},       false,  0.0f, &ice_elf_texture,    glm::vec2{32.0f, 32.0f}},
	{"bridge", "bridge", "bridge_fixing", ItemRecord{"", 0}, ItemRecord{"", 0},                true,   3.0f, &woodbridge_texture, glm::vec2{32.0f, 32.0f}},
	{"waystone", "waystone", "", ItemRecord{"", 0}, ItemRecord{"", 0}, true,   100.0f, &waystone_texture, glm::vec2{32.0f, 32.0f}},
	{"minerguy", "minerguy", "miner_crystal", ItemRecord{"flower_crystal", 3}, ItemRecord{"feather", 1}, true,   100.0f, &lumberjack_texture, glm::vec2{32.0f, 32.0f}},
};

NpcDef const *NpcDef::get(std::string const &type) {
	for (NpcDef const &d : npc_defs) {
		if (d.type == type) return &d;
	}
	return nullptr;
}

//how close the dwarf has to be for the npc to turn toward them (pixels)
static constexpr float LOOK_DISTANCE = 3.0f * float(GameConfig::TILE_SIZE);
//seconds per idle frame (slow breathing)
static constexpr float IDLE_FRAME_SECONDS = 0.7f;

Npc::Npc(std::string const &type) {
	set_def(type);
}

void Npc::set_def(std::string const &type) {
	def = NpcDef::get(type);
	if (def == nullptr) throw std::runtime_error{"Unknown npc type: " + type};
	//trade panel: slot 0 only takes what the npc wants, slot 1 is take-only
	trade.set_rule(0, Inventory::SlotRule{.required_item = def->trade_in.first});
	trade.set_rule(1, Inventory::SlotRule{.can_insert = false});
}

void Npc::face(glm::vec2 point) {
	glm::vec2 d = point - position;
	if (std::abs(d.x) > std::abs(d.y)) facing = glm::ivec2(d.x > 0.0f ? 1 : -1, 0);
	else facing = glm::ivec2(0, d.y > 0.0f ? 1 : -1);
}

void Npc::update(FrameInput const &input, float elapsed, World const &world) {
	idle_timer += elapsed;
	if (idle_timer > 1000.0f) idle_timer -= 1000.0f;

	//turn toward the dwarf while they're close, otherwise look down
	if (glm::length(world.player.position - position) <= LOOK_DISTANCE) face(world.player.position);
	else facing = glm::ivec2(0, 1);

	//trade: trade_in in slot 0 becomes trade_out in slot 1, as long as it fits.
	//It keeps going while the panel is closed, like a machine.
	if (!def->trades()) return;
	auto can_trade = [this]() {
		if (trade.get_slot_count(0) < def->trade_in.second) return false;
		int space = trade.slot_empty(1) ? Inventory::MAX_STACK
			: (trade.get_slot_id(1) == def->trade_out.first ? Inventory::MAX_STACK - trade.get_slot_count(1) : 0);
		return space >= def->trade_out.second;
	};
	auto make_one = [this]() {
		trade.extract_from_slot(0, def->trade_in.second);
		trade.add_to_slot_range(def->trade_out.first, def->trade_out.second, 1, 2);
	};

	if (def->trade_seconds <= 0.0f) {
		while (can_trade()) make_one(); //instant: everything at once
		return;
	}
	if (!can_trade()) {
		trade_timer = 0.0f;
		return;
	}
	trade_timer += elapsed;
	if (trade_timer >= def->trade_seconds) {
		trade_timer = 0.0f;
		make_one();
	}
}

void Npc::on_interact(Context &ctx) {
	face(ctx.player.position);
}

float Npc::get_progress() const {
	if (def->trade_seconds <= 0.0f) return trade.slot_empty(1) ? 0.0f : 1.0f;
	return std::min(1.0f, trade_timer / def->trade_seconds);
}

void Npc::draw(Sprites &batch) const {
	int row = 3; //down
	if (facing.y < 0) row = 1;      //up
	else if (facing.x < 0) row = 2; //left
	else if (facing.x > 0) row = 0; //right
	int column = int(idle_timer / IDLE_FRAME_SECONDS) % 2;

	float w = def->frame_size.x, h = def->frame_size.y;
	glm::vec2 top_left = glm::round(position) - glm::vec2(w * 0.5f, h);
	batch.draw(**def->texture, Rect{w * float(column), h * float(row), w, h}, top_left);
}

glm::vec2 Npc::hint_anchor() const {
	return position - glm::vec2(0.0f, def->frame_size.y - 1.0f);
}

glm::vec2 Npc::marker_anchor() const {
	return position - glm::vec2(0.0f, def->frame_size.y + 1.0f);
}

auto Npc::hitbox() const -> Rect {
	return Rect{position.x - 7.0f, position.y - 8.0f, 14.0f, 8.0f};
}

void Npc::get_occupied(std::vector<glm::ivec2> *ans) const {
	ans->emplace_back(Tilemap::tile_at_point(position - glm::vec2(0.0f, 1.0f)));
}

std::string Npc::save() const {
	std::ostringstream ss;
	ss << uid << ' ' << def->type << ' ' << position.x << ' ' << position.y << ' ' << (met ? 1 : 0);
	for (size_t i = 0; i < trade.size(); ++i) {
		item_id_t id = trade.slot_empty(i) ? "null" : trade[i].first;
		ss << ' ' << id << ' ' << (trade.slot_empty(i) ? 0 : trade[i].second);
	}
	return ss.str();
}

void Npc::load(std::string line) {
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
	//the rest is optional (a fresh level file has just the position)
	int read_met = 0;
	if (ss >> read_met) met = (read_met != 0);
	for (size_t i = 0; i < trade.size(); ++i) {
		item_id_t id;
		int count = 0;
		if (!(ss >> id >> count)) break;
		if (id == "null" || count <= 0) trade.set_slot(i, "", 0);
		else trade.set_slot(i, id, count);
	}
}