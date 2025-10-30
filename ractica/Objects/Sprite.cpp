#include "../pch.h"
#include "Sprite.h"

Sprite::Sprite(glm::vec3 pos, glm::vec3 col, float s, float sp)
    : position(pos), color(col), size(s), speed(sp) {
}