#include "AppleTree.hpp"
#include "../World.hpp"
#include "../TileMap.hpp"
#include "../ItemEntity.hpp"
#include "../../starter/Load.hpp"
#include "../../starter/data_path.hpp"

#include <cmath>
#include <array>
#include <random>

static Load< Texture > box_texture(LoadTagDefault, []() -> Texture const * {
	return new Texture(data_path("assets/test/apple_tree.png"));
});

static std::mt19937 tree_rng(0x6170706c); // "appl"

glm::vec2 AppleTree::base_center() const {
    return position + glm::vec2(GameConfig::TILE_SIZE, 0.5f * GameConfig::TILE_SIZE);
}

void AppleTree::update(FrameInput const &input, float elapsed, World const &world) {
    glm::vec2 center = base_center();

    // 1. count the apples already lying around the tree
    nearby_apples = 0;
    for (auto const &entity : world.entities) {
        if (entity->removed) continue;
        auto *item = dynamic_cast< ItemEntity * >(entity.get());
        if (item == nullptr) continue;
        if (item->get_inventory()->get_slot_id(0) != "apple") continue;
        if (glm::length(item->position - center) > COUNT_RADIUS) continue;
        nearby_apples += item->get_inventory()->get_slot_count(0);
    }

    // 2. every DROP_COOLDOWN seconds, drop one (unless there are enough already)
    drop_timer -= elapsed;
    if (drop_timer > 0.0f) return;
    drop_timer = DROP_COOLDOWN;
    if (nearby_apples >= MAX_NEARBY_APPLES || world.spawned == nullptr) return;

    // 3. pick a random spot 1..2 tiles from the tree that isn't a wall (give up after a few tries)
    std::uniform_real_distribution< float > unit(0.0f, 1.0f);
    for (int attempt = 0; attempt < 8; ++attempt) {
        float angle = unit(tree_rng) * 6.2831853f;
        float distance = (0.5f + 0.5f * unit(tree_rng)) * DROP_RADIUS;
        glm::vec2 spot = center + distance * glm::vec2(std::cos(angle), std::sin(angle));
        if (world.map.is_solid(Tilemap::tile_at_point(spot))) continue;

        auto apple = std::make_unique< ItemEntity >("apple", 1);
        apple->position = spot;
        world.spawned->push_back(std::move(apple));
        nearby_apples += 1;
        break;
    }
}

void AppleTree::draw(Sprites &batch) const {
    bool has_fruit = nearby_apples < MAX_NEARBY_APPLES;
    Rect src{has_fruit ? 32.0f : 0.0f, 0.0f, 32.0f, 48.0f};
    //round to whole pixels so the sprite never sits between pixels:
    glm::vec2 top_left = glm::round(position) - glm::vec2(0.0f, 32.0f);
    batch.draw(*box_texture, src, top_left);

}

auto AppleTree::hitbox() const -> Rect {
    return Rect{position.x, position.y, 32.0f, 16.0f};
}

void AppleTree::get_occupied(std::vector<glm::ivec2> *ans) const {
    auto tile = Tilemap::tile_at_point(position + glm::vec2{GameConfig::TILE_SIZE / 2.0f});
	ans->emplace_back(tile);
    ans->emplace_back(tile + glm::ivec2{1, 0});
}

void AppleTree::on_interact(Context &ctx) {
    // should trigger inventory display through some mechanism
    ctx.open_container = &inventory;
}

auto AppleTree::get_inventory() -> Inventory* {
    return &inventory;
}
