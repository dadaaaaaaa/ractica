#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <iostream>

// Forward declaration
struct Ray;

// Структура для хранения индексов объектов в ячейке
struct GridCell {
    std::vector<int> snakeIndices;
    std::vector<int> foodIndices;
    std::vector<int> obstacleIndices;
    std::vector<int> fenceIndices;
    std::vector<int> birdIndices;
    std::vector<int> cloudIndices;
    std::vector<int> flowerIndices;

    void clear() {
        snakeIndices.clear();
        foodIndices.clear();
        obstacleIndices.clear();
        fenceIndices.clear();
        birdIndices.clear();
        cloudIndices.clear();
        flowerIndices.clear();
    }

    bool hasAny() const {
        return !snakeIndices.empty() || !foodIndices.empty() ||
            !obstacleIndices.empty() || !fenceIndices.empty() ||
            !birdIndices.empty() || !cloudIndices.empty() ||
            !flowerIndices.empty();
    }
};

// Основной класс Grid-акселератора
class GridAccelerator {
private:
    struct GridData {
        std::vector<GridCell> cells;
        glm::vec3 worldMin;
        glm::vec3 worldMax;
        glm::ivec3 gridSize;
        glm::vec3 cellSize;

        int getCellIndex(int x, int y, int z) const {
            return (z * gridSize.y * gridSize.x) + (y * gridSize.x) + x;
        }

        bool isValidCell(int x, int y, int z) const {
            return x >= 0 && x < gridSize.x &&
                y >= 0 && y < gridSize.y &&
                z >= 0 && z < gridSize.z;
        }
    };

    GridData m_data;
    bool m_enabled;
    bool m_dirty;
    int m_totalObjects;

    // Простой метод для добавления объекта в ячейки
    void addToGrid(const glm::vec3& minBB, const glm::vec3& maxBB,
        int objectId, int type);

public:
    GridAccelerator();

    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }
    void setGridSize(const glm::ivec3& size) { m_data.gridSize = size; m_dirty = true; }

    void build(float worldWidth, float worldDepth, float worldHeight);
    void clear();
    void markDirty() { m_dirty = true; }
    bool isDirty() const { return m_dirty; }

    // Добавление объектов
    void addSnakeSegment(int segmentId, const glm::vec3& position, float halfSize);
    void addFood(int foodId, const glm::vec3& position, float radius);
    void addObstacle(int obstacleId, const glm::vec3& position, float halfWidth, float height);
    void addFenceBlock(int fenceId, const glm::vec3& position, float halfSize, float height);
    void addBird(int birdId, const glm::vec3& position, float radius);
    void addCloud(int cloudId, const glm::vec3& position, float radius);
    void addFlower(int flowerId, const glm::vec3& position, float radius);

    // Обход сетки лучом
    void traverseRay(const Ray& ray, float maxDistance,
        std::vector<int>& snakeIndices,
        std::vector<int>& foodIndices,
        std::vector<int>& obstacleIndices,
        std::vector<int>& fenceIndices,
        std::vector<int>& birdIndices,
        std::vector<int>& cloudIndices,
        std::vector<int>& flowerIndices) const;

    void printStats() const;
    size_t getTotalCells() const { return m_data.cells.size(); }
    size_t getNonEmptyCells() const;
    float getFillRatio() const;
};