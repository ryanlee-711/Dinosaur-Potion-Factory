#pragma once

#include "../Entity.hpp"
#include "../Components.hpp"
#include "../../main/Rect.hpp"
#include "../../rendering/Sprites.hpp"

#include <glm/glm.hpp>

class Boulder : public Entity, public IInteractable, public IBreakable, public IHintable {
    /* Boulder implementation for demo. */
public:
    Boulder(bool use_big = true);
    ~Boulder() = default;

    // Entity
    void update(FrameInput const &input, float elapsed, World const &world) override;
	auto hitbox() const -> Rect override;
    void draw(Sprites &batch) const override;
    void get_occupied(std::vector<glm::ivec2> *ans) const override;

    std::string save() const override;
    void load(std::string line) override;

    // IInteractable
    void on_interact(Context &ctx) override;

    // IBreakable
    static constexpr int BIG_HARDNESS = 8, SMALL_HARDNESS = 4;
    int get_hardness() override {return big ? BIG_HARDNESS : SMALL_HARDNESS;}
    void on_break_hit() override { shake_timer = SHAKE_SECONDS; }    // wobble on each pickaxe hit
    ParticlePreset const *break_hit_particles() const override;    // rock chips on each pickaxe hit
    static constexpr float SHAKE_SECONDS = 0.15f;
    item_id_t get_drop_id() override {return "rock";}

    // IHintable: small = E key (push it, needs any strength), big = pickaxe icon. Faded until the dwarf has that strength.
    bool show_hint() const override { return true; }
    item_id_t hint_tool() const override { return big ? "pickaxe" : ""; }
    glm::vec2 hint_anchor() const override;
    bool hint_locked(Player const &player) const override;

    bool big = true;

private:
    bool rolling = false;
    glm::ivec2 target_tile = glm::ivec2{0};
    glm::vec2 sprite_offset = glm::vec2{0};
    float shake_timer = 0.0f;      // > 0: wobbling from a pickaxe hit

};