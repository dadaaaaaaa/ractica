#include "../pch.h"
#include "MathUtils.h"

namespace MathUtils {
    glm::vec3 lerp(const glm::vec3& a, const glm::vec3& b, float t) {
        return a + t * (b - a);
    }

    float lerp(float a, float b, float t) {
        return a + t * (b - a);
    }
    inline float clamp(float value, float min, float max) {
        if (value < min) return min;
        if (value > max) return max;
        return value;
    }

    // Преобразование координат сетки в мировые координаты
    inline float gridToWorld(int gridCoord, int gridSize) {
        return (gridCoord - gridSize / 2.0f) * CELL_SIZE;
    }

    // Преобразование мировых координат в координаты сетки
    inline int worldToGrid(float worldCoord, int gridSize) {
        return static_cast<int>((worldCoord / CELL_SIZE) + gridSize / 2.0f);
    }
}