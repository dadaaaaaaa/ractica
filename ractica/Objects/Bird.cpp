#include "../pch.h"
#include "Bird.h"

Bird::Bird(glm::vec3 pos, glm::vec3 col, float s, float sp)
    : position(pos), color(col), size(s), speed(sp),
    wingAngle(0.0f), wingSpeed(5.0f + (rand() % 10) * 0.5f),
    direction(glm::vec3(0.0f)) {
}