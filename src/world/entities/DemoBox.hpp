#pragma once

#include "../Entity.hpp"
#include "../../main/Rect.hpp"
#include "../../rendering/Sprites.hpp"

#include <glm/glm.hpp>

#include "../../rendering/Texture.hpp"

struct DemoBox : Entity {

    ~DemoBox() = default;

    void update(FrameInput const &input, float elapsed, Tilemap const &map) override;
	Rect hitbox() const override;
    void draw(Sprites &batch) const override;

private:

};