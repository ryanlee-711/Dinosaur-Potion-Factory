#pragma once

#include "../Entity.hpp"
#include "../../main/Rect.hpp"
#include "../../rendering/Sprites.hpp"
#include "../../rendering/Texture.hpp"
#include "../../starter/Load.hpp"

#include <glm/glm.hpp>

#include <string>
#include <vector>

// Obstacles and buildings that just sit there. No interaction.
// To add a new one: add a texture and one line to the table in Prop.cpp, then its type name to the prop branch in PlayMode::load.
struct PropDef {
    std::string type;          // name in the save file
    glm::ivec2 tiles;          // footprint in tiles, from the tile at 'position' (hitbox)
    glm::vec2 sprite_size;     // in pixels. the sprite's bottom sits on the footprint's bottom
    float sprite_x;            // sprite's left edge relative to the footprint's left edge, in pixels
    bool solid;                // false = you can walk over it
    Load< Texture > *texture;

    static PropDef const *get(std::string const &type);
};

class Prop : public Entity {
public:
    explicit Prop(std::string const &type = "woodpile");
    ~Prop() = default;

    // Entity
    void update(FrameInput const &input, float elapsed, World const &world) override { }
    auto hitbox() const -> Rect override;
    void draw(Sprites &batch) const override;
    void get_occupied(std::vector<glm::ivec2> *ans) const override;

    // Save line: <uid> <type> <x> <y>   (position = top-left of the footprint's first tile)
    std::string save() const override;
    void load(std::string line) override;

private:
    void set_def(std::string const &type);
    PropDef const *def = nullptr;
};