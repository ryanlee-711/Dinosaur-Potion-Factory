#pragma once

#include "../Entity.hpp"
#include "../Components.hpp"
#include "../../main/Rect.hpp"
#include "../../rendering/Sprites.hpp"
#include "../../rendering/Texture.hpp"
#include "../../starter/Load.hpp"

#include <glm/glm.hpp>

struct ParticlePreset;

#include <string>
#include <vector>

// One kind of ore vein. To add a new ore: add a texture and one line to the table in OreVein.cpp,
// then a branch in PlayMode::load for its type name.
struct OreVeinDef {
    std::string type;          // name in the save file
    item_id_t drop;            // what one pickaxe click knocks loose
    int max_ores;              // max ores it holds
    float regrow_seconds;      // seconds to grow back one ore
    Load< Texture > *texture;  // 64x32: frame 0 = mined out, frame 1 = has ore
    ParticlePreset const *hit_particles; // chips sprayed on every pickaxe hit

    static OreVeinDef const *get(std::string const &type);
};

class OreVein : public Entity, public IMineable, public IHintable {
public:
    explicit OreVein(std::string const &type = "copper_vein");
    ~OreVein() = default;

    // Entity
    void update(FrameInput const &input, float elapsed, World const &world) override;
    auto hitbox() const -> Rect override;
    void draw(Sprites &batch) const override;
    void get_occupied(std::vector<glm::ivec2> *ans) const override;

    // Save line: <uid> <type> <x> <y> <ores>   (load also sets the type, so one class loads every ore)
    std::string save() const override;
    void load(std::string line) override;

    // IMineable
    item_id_t required_tool() const override { return "pickaxe"; }
    bool mine(int power, glm::vec2 from, std::vector<ItemRecord> &drops) override;
    bool restore(); // refill
    static constexpr float SHAKE_SECONDS = 0.15f; // wobble after each hit, like trees

    std::string empty_message() const override { return "Nothing left to mine. It grows back slowly"; }

    // IHintable: pickaxe icon while there's ore
    bool show_hint() const override { return ores > 0; }
    item_id_t hint_tool() const override { return required_tool(); }
    glm::vec2 hint_anchor() const override { return position + glm::vec2(16.0f, -17.0f); } //above the 32x32 sprite

private:
    OreVeinDef const *def = nullptr;
    int ores = 0;               // ores left to mine right now
    float shake_timer = 0.0f;   // > 0: wobbling from a pickaxe hit
    bool chips_pending = false; // a hit landed: spray chips next update (needs the World for particles)

    float regrow_timer = 0.0f;
};