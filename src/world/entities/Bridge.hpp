#pragma once
#include "Npc.hpp"

class Bridge : public Npc {
public:
    Bridge() : Npc("bridge") {}

    void update(FrameInput const &, float, World const &) override {}
    void on_interact(Context &) override {}

    void draw(Sprites &batch) const override;
    Rect hitbox() const override;
    void get_occupied(std::vector<glm::ivec2> *out) const override;

    void repair() {
        repaired = true;
        collision_layer = 0;
    }

private:
    bool repaired = false;
};