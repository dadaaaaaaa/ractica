#pragma once
#include <glm/glm.hpp>
#include "../Core/Constants.h"
namespace MathUtils {
    glm::vec3 lerp(const glm::vec3& a, const glm::vec3& b, float t);
    float lerp(float a, float b, float t);
    inline float clamp(float value, float min, float max);
    inline float gridToWorld(int gridCoord, int gridSize);
    inline int worldToGrid(float worldCoord, int gridSize);
    
}