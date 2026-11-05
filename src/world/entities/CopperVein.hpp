#pragma once

#include "../Entity.hpp"
#include "../Components.hpp"
#include "../../main/Rect.hpp"
#include "../../rendering/Sprites.hpp"

#include <glm/glm.hpp>

class CopperVein : public Entity, public IMineable {
    /* A copper vein: click it with the pickaxe to knock loose one copper ore (it drops on the ground).
       It holds a few ores and slowly regrows them, so it can't be mined endlessly. */
public:
    CopperVein() = default;
    ~CopperVein() = default;

    // Entity
    void update(FrameInput const &input, float elapsed, World const &world) override;
	auto hitbox() const -> Rect override;
    void draw(Sprites &batch) const override;
    void get_occupied(std::vector<glm::ivec2> *ans) const override;

    // IMineable
    item_id_t mine() override;

private:
    static constexpr int MAX_ORES = 4;              // ores the vein can hold at once
    static constexpr float REGROW_COOLDOWN = 6.0f;  // seconds to grow back one ore
    int ores = MAX_ORES;                            // ores left to mine right now
    float regrow_timer = REGROW_COOLDOWN;
};