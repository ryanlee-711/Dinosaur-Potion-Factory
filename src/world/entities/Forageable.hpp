#pragma once

#include "../Entity.hpp"
#include "../Components.hpp"
#include "../../main/Rect.hpp"
#include "../../rendering/Sprites.hpp"
#include "../../rendering/Texture.hpp"
#include "../../starter/Load.hpp"

#include <glm/glm.hpp>

#include <string>
#include <vector>

// To add a new one: add a texture and one line to the table in Forageable.cpp,
// then a branch in PlayMode::load for its type name.
struct ForageableDef {
    std::string type;          // name in the save file
    item_id_t drop;            // what pressing E picks up
    int max_count;             // how many it holds at once (E picks them all)
    float regrow_seconds;      // seconds to grow back one
    glm::ivec2 tiles;          // footprint in tiles (hitbox)
    glm::vec2 sprite_size;     // one frame, in pixels, the sprite's bottom sits on the footprint's bottom
    bool solid;                // false = you can walk over it
    std::string empty_message; // shown when there's nothing to pick
    Load< Texture > *texture;  // frame 0 = picked bare, frame 1 = ready to pick

    static ForageableDef const *get(std::string const &type);
};

class Forageable : public Entity, public IInteractable {
public:
    explicit Forageable(std::string const &type = "lichen_rock");
    ~Forageable() = default;

    // Entity
    void update(FrameInput const &input, float elapsed, World const &world) override;
    auto hitbox() const -> Rect override;
    void draw(Sprites &batch) const override;
    void get_occupied(std::vector<glm::ivec2> *ans) const override;

    // Save line: <uid> <type> <x> <y> <count>   (load also sets the type, so one class loads every kind)
    std::string save() const override;
    void load(std::string line) override;

    // IInteractable
    void on_interact(Context &ctx) override;

private:
    void set_def(std::string const &type);

    ForageableDef const *def = nullptr;
    int count = 0;              // ready to pick right now
    float regrow_timer = 0.0f;
};