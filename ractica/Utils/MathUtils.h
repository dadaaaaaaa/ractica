#pragma once
#include <glm/glm.hpp>

namespace MathUtils {
    glm::vec3 lerp(const glm::vec3& a, const glm::vec3& b, float t);
    float lerp(float a, float b, float t);
}