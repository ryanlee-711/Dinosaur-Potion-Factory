#pragma once

#include "../Entity.hpp"
#include "../Components.hpp"
#include "../../main/Rect.hpp"
#include "../../rendering/Sprites.hpp"

#include <glm/glm.hpp>

class Boulder : public Entity, public IInteractable, public IBreakable {
    /* Boulder implementation for demo. */
public:
    Boulder(bool use_big = true);
    ~Boulder() = default;

    // Entity
    void update(FrameInput const &input, float elapsed, World const &world) override;
	auto hitbox() const -> Rect override;
    void draw(Sprites &batch) const override;
    void get_occupied(std::vector<glm::ivec2> *ans) const override;

    // IInteractable
    void on_interact(Context &ctx) override;

    // IBreakable
    int get_hardness() override {return big ? 8 : 6;}
    item_id_t get_drop_id() override {return "";}

    bool big = true;

private:
    bool rolling = false;
    glm::ivec2 target_tile = glm::ivec2{0};
    glm::vec2 sprite_offset = glm::vec2{0};

};