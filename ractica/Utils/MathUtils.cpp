#include "../pch.h"
#include "MathUtils.h"


namespace MathUtils {
    // ============================================================================
    // ЛИНЕЙНАЯ ИНТЕРПОЛЯЦИЯ
    // ============================================================================

    glm::vec3 lerp(const glm::vec3& a, const glm::vec3& b, float t) {
        t = clamp(t, 0.0f, 1.0f);
        return a + t * (b - a);
    }

    float lerp(float a, float b, float t) {
        t = clamp(t, 0.0f, 1.0f);
        return a + t * (b - a);
    }

    // ============================================================================
    // ОГРАНИЧЕНИЕ ЗНАЧЕНИЙ
    // ============================================================================

    float clamp(float value, float min, float max) {
        if (value < min) return min;
        if (value > max) return max;
        return value;
    }

    int clamp(int value, int min, int max) {
        if (value < min) return min;
        if (value > max) return max;
        return value;
    }

    // ============================================================================
    // ПРЕОБРАЗОВАНИЕ КООРДИНАТ СЕТКА ↔ МИР
    // ============================================================================

    float gridToWorld(int gridCoord, int gridSize) {
        return (gridCoord - gridSize / 2.0f) * CELL_SIZE;
    }

    int worldToGrid(float worldCoord, int gridSize) {
        return static_cast<int>((worldCoord / CELL_SIZE) + gridSize / 2.0f);
    }

    glm::vec3 gridPointToWorld(const Point& gridPoint, int gridWidth, int gridDepth) {
        return glm::vec3(
            gridToWorld(gridPoint.x, gridWidth),
            gridPoint.y * CELL_SIZE, // Высота также масштабируется
            gridToWorld(gridPoint.z, gridDepth)
        );
    }

    Point worldToGridPoint(const glm::vec3& worldPos, int gridWidth, int gridDepth) {
        return Point(
            worldToGrid(worldPos.x, gridWidth),
            static_cast<int>(worldPos.y / CELL_SIZE), // Обратное преобразование высоты
            worldToGrid(worldPos.z, gridDepth)
        );
    }

    // ============================================================================
    // РАССТОЯНИЯ И ГЕОМЕТРИЯ
    // ============================================================================

    float gridDistance(const Point& a, const Point& b) {
        int dx = a.x - b.x;
        int dz = a.z - b.z;
        return std::sqrt(static_cast<float>(dx * dx + dz * dz));
    }

    int gridDistanceSquared(const Point& a, const Point& b) {
        int dx = a.x - b.x;
        int dz = a.z - b.z;
        return dx * dx + dz * dz;
    }

    bool isInGridBounds(const Point& point, int gridWidth, int gridDepth) {
        return point.x >= 0 && point.x < gridWidth &&
            point.z >= 0 && point.z < gridDepth &&
            point.y >= 0 && point.y < GRID_HEIGHT;
    }

    bool isInGridBounds(int x, int z, int gridWidth, int gridDepth) {
        return x >= 0 && x < gridWidth && z >= 0 && z < gridDepth;
    }

    // ============================================================================
    // РАБОТА С УГЛАМИ
    // ============================================================================

    float normalizeAngle(float angle) {
        while (angle < 0.0f) angle += 2.0f * 3.14159265359f;
        while (angle >= 2.0f * 3.14159265359f) angle -= 2.0f * 3.14159265359f;
        return angle;
    }

    float degreesToRadians(float degrees) {
        return degrees * 3.14159265359f / 180.0f;
    }

    float radiansToDegrees(float radians) {
        return radians * 180.0f / 3.14159265359f;
    }

    glm::vec3 angleToDirection(float angle) {
        return glm::vec3(
            std::cos(angle),
            0.0f,
            std::sin(angle)
        );
    }

    float directionToAngle(const glm::vec3& direction) {
        return std::atan2(direction.z, direction.x);
    }

    // ============================================================================
    // СЛУЧАЙНЫЕ ЧИСЛА
    // ============================================================================

    float randomFloat(float min, float max) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dis(min, max);
        return dis(gen);
    }

    int randomInt(int min, int max) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dis(min, max);
        return dis(gen);
    }

    // Генерация случайной точки в пределах сетки
    Point randomGridPoint(int gridWidth, int gridDepth) {
        return Point(
            randomInt(0, gridWidth - 1),
            0, // По умолчанию на уровне земли
            randomInt(0, gridDepth - 1)
        );
    }

    Point randomGridPoint(int gridWidth, int gridDepth, int margin) {
        return Point(
            randomInt(margin, gridWidth - 1 - margin),
            0,
            randomInt(margin, gridDepth - 1 - margin)
        );
    }

    // ============================================================================
    // УТИЛИТЫ
    // ============================================================================

    int roundToInt(float value) {
        return static_cast<int>(std::round(value));
    }

    bool approximatelyEqual(float a, float b, float epsilon) {
        return std::abs(a - b) <= epsilon;
    }

    // Проверка столкновения двух AABB (Axis-Aligned Bounding Box)
    bool checkAABBCollision(const glm::vec3& pos1, const glm::vec3& size1,
        const glm::vec3& pos2, const glm::vec3& size2) {
        return (pos1.x - size1.x / 2.0f < pos2.x + size2.x / 2.0f &&
            pos1.x + size1.x / 2.0f > pos2.x - size2.x / 2.0f &&
            pos1.z - size1.z / 2.0f < pos2.z + size2.z / 2.0f &&
            pos1.z + size1.z / 2.0f > pos2.z - size2.z / 2.0f);
    }

    // Вычисление нормализованного направления между двумя точками
    glm::vec3 directionBetween(const glm::vec3& from, const glm::vec3& to) {
        return glm::normalize(to - from);
    }

    // Вычисление расстояния между двумя точками в 3D пространстве
    float distance(const glm::vec3& a, const glm::vec3& b) {
        return glm::distance(a, b);
    }

    // Вычисление квадрата расстояния (более эффективно)
    float distanceSquared(const glm::vec3& a, const glm::vec3& b) {
        glm::vec3 diff = a - b;
        return glm::dot(diff, diff);
    }

    // Плавное движение с эйзинговой функцией
    float easeInOut(float t) {
        return t * t * (3.0f - 2.0f * t);
    }

    float easeIn(float t) {
        return t * t;
    }

    float easeOut(float t) {
        return 1.0f - (1.0f - t) * (1.0f - t);
    }
}