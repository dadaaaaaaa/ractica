#include "../pch.h"
#include "MathUtils.h"

namespace MathUtils {
    glm::vec3 lerp(const glm::vec3& a, const glm::vec3& b, float t) {
        return a + t * (b - a);
    }

    float lerp(float a, float b, float t) {
        return a + t * (b - a);
    }
}