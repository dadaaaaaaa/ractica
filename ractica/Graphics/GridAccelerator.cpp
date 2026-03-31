#include "GridAccelerator.h"
#include "RayTracer.h"
#include <algorithm>
#include <cmath>

// Константы для типов объектов
enum ObjectType {
    TYPE_SNAKE = 0,
    TYPE_FOOD = 1,
    TYPE_OBSTACLE = 2,
    TYPE_FENCE = 3,
    TYPE_BIRD = 4,
    TYPE_CLOUD = 5,
    TYPE_FLOWER = 6
};

GridAccelerator::GridAccelerator()
    : m_enabled(true)
    , m_dirty(true)
    , m_totalObjects(0)
{
    m_data.gridSize = glm::ivec3(40, 20, 40);
}

void GridAccelerator::build(float worldWidth, float worldDepth, float worldHeight) {
    if (!m_enabled) return;

    m_data.worldMin = glm::vec3(-worldWidth / 2.0f, 0.0f, -worldDepth / 2.0f);
    m_data.worldMax = glm::vec3(worldWidth / 2.0f, worldHeight, worldDepth / 2.0f);

    m_data.cellSize = glm::vec3(
        (m_data.worldMax.x - m_data.worldMin.x) / m_data.gridSize.x,
        (m_data.worldMax.y - m_data.worldMin.y) / m_data.gridSize.y,
        (m_data.worldMax.z - m_data.worldMin.z) / m_data.gridSize.z
    );

    int totalCells = m_data.gridSize.x * m_data.gridSize.y * m_data.gridSize.z;
    m_data.cells.clear();
    m_data.cells.resize(totalCells);

    m_totalObjects = 0;
    m_dirty = false;

    std::cout << "GridAccelerator: Built " << totalCells << " cells, "
        << "cell size = (" << m_data.cellSize.x << ", "
        << m_data.cellSize.y << ", " << m_data.cellSize.z << ")" << std::endl;
}

void GridAccelerator::clear() {
    for (auto& cell : m_data.cells) {
        cell.clear();
    }
    m_totalObjects = 0;
    m_dirty = true;
}

void GridAccelerator::addToGrid(const glm::vec3& minBB, const glm::vec3& maxBB,
    int objectId, int type) {
    // Вычисляем ячейки, которые пересекает bounding box
    int minX = (int)((minBB.x - m_data.worldMin.x) / m_data.cellSize.x);
    int minY = (int)((minBB.y - m_data.worldMin.y) / m_data.cellSize.y);
    int minZ = (int)((minBB.z - m_data.worldMin.z) / m_data.cellSize.z);

    int maxX = (int)((maxBB.x - m_data.worldMin.x) / m_data.cellSize.x);
    int maxY = (int)((maxBB.y - m_data.worldMin.y) / m_data.cellSize.y);
    int maxZ = (int)((maxBB.z - m_data.worldMin.z) / m_data.cellSize.z);

    // Ограничиваем границами сетки
    minX = std::max(0, minX);
    minY = std::max(0, minY);
    minZ = std::max(0, minZ);
    maxX = std::min(m_data.gridSize.x - 1, maxX);
    maxY = std::min(m_data.gridSize.y - 1, maxY);
    maxZ = std::min(m_data.gridSize.z - 1, maxZ);

    // Добавляем объект во все пересекаемые ячейки
    for (int x = minX; x <= maxX; x++) {
        for (int y = minY; y <= maxY; y++) {
            for (int z = minZ; z <= maxZ; z++) {
                int idx = m_data.getCellIndex(x, y, z);
                GridCell& cell = m_data.cells[idx];

                switch (type) {
                case TYPE_SNAKE:
                    cell.snakeIndices.push_back(objectId);
                    break;
                case TYPE_FOOD:
                    cell.foodIndices.push_back(objectId);
                    break;
                case TYPE_OBSTACLE:
                    cell.obstacleIndices.push_back(objectId);
                    break;
                case TYPE_FENCE:
                    cell.fenceIndices.push_back(objectId);
                    break;
                case TYPE_BIRD:
                    cell.birdIndices.push_back(objectId);
                    break;
                case TYPE_CLOUD:
                    cell.cloudIndices.push_back(objectId);
                    break;
                case TYPE_FLOWER:
                    cell.flowerIndices.push_back(objectId);
                    break;
                }
            }
        }
    }
}

void GridAccelerator::addSnakeSegment(int segmentId, const glm::vec3& position, float halfSize) {
    if (!m_enabled) return;
    glm::vec3 minBB = position - glm::vec3(halfSize);
    glm::vec3 maxBB = position + glm::vec3(halfSize);
    addToGrid(minBB, maxBB, segmentId, TYPE_SNAKE);
    m_totalObjects++;
}

void GridAccelerator::addFood(int foodId, const glm::vec3& position, float radius) {
    if (!m_enabled) return;
    glm::vec3 minBB = position - glm::vec3(radius);
    glm::vec3 maxBB = position + glm::vec3(radius);
    addToGrid(minBB, maxBB, foodId, TYPE_FOOD);
    m_totalObjects++;
}

void GridAccelerator::addObstacle(int obstacleId, const glm::vec3& position, float halfWidth, float height) {
    if (!m_enabled) return;
    glm::vec3 minBB = position - glm::vec3(halfWidth, 0.0f, halfWidth);
    glm::vec3 maxBB = position + glm::vec3(halfWidth, height, halfWidth);
    addToGrid(minBB, maxBB, obstacleId, TYPE_OBSTACLE);
    m_totalObjects++;
}

void GridAccelerator::addFenceBlock(int fenceId, const glm::vec3& position, float halfSize, float height) {
    if (!m_enabled) return;
    glm::vec3 minBB = position - glm::vec3(halfSize, 0.0f, halfSize);
    glm::vec3 maxBB = position + glm::vec3(halfSize, height, halfSize);
    addToGrid(minBB, maxBB, fenceId, TYPE_FENCE);
    m_totalObjects++;
}

void GridAccelerator::addBird(int birdId, const glm::vec3& position, float radius) {
    if (!m_enabled) return;
    glm::vec3 minBB = position - glm::vec3(radius);
    glm::vec3 maxBB = position + glm::vec3(radius);
    addToGrid(minBB, maxBB, birdId, TYPE_BIRD);
    m_totalObjects++;
}

void GridAccelerator::addCloud(int cloudId, const glm::vec3& position, float radius) {
    if (!m_enabled) return;
    glm::vec3 minBB = position - glm::vec3(radius);
    glm::vec3 maxBB = position + glm::vec3(radius);
    addToGrid(minBB, maxBB, cloudId, TYPE_CLOUD);
    m_totalObjects++;
}

void GridAccelerator::addFlower(int flowerId, const glm::vec3& position, float radius) {
    if (!m_enabled) return;
    glm::vec3 minBB = position - glm::vec3(radius);
    glm::vec3 maxBB = position + glm::vec3(radius);
    addToGrid(minBB, maxBB, flowerId, TYPE_FLOWER);
    m_totalObjects++;
}

void GridAccelerator::traverseRay(const Ray& ray, float maxDistance,
    std::vector<int>& snakeIndices,
    std::vector<int>& foodIndices,
    std::vector<int>& obstacleIndices,
    std::vector<int>& fenceIndices,
    std::vector<int>& birdIndices,
    std::vector<int>& cloudIndices,
    std::vector<int>& flowerIndices) const {

    if (!m_enabled || m_data.cells.empty()) return;

    glm::vec3 dir = ray.direction;
    glm::vec3 safeDir = dir;
    if (std::abs(safeDir.x) < 1e-6f) safeDir.x = 1e-6f;
    if (std::abs(safeDir.y) < 1e-6f) safeDir.y = 1e-6f;
    if (std::abs(safeDir.z) < 1e-6f) safeDir.z = 1e-6f;

    // Находим начальную ячейку
    glm::ivec3 cell;
    for (int i = 0; i < 3; i++) {
        cell[i] = (int)((ray.origin[i] - m_data.worldMin[i]) / m_data.cellSize[i]);
        if (cell[i] < 0) cell[i] = 0;
        if (cell[i] >= m_data.gridSize[i]) cell[i] = m_data.gridSize[i] - 1;
    }

    // DDA параметры
    glm::ivec3 step;
    glm::vec3 tDelta;
    glm::vec3 tMax;

    for (int i = 0; i < 3; i++) {
        if (safeDir[i] > 0) {
            step[i] = 1;
            float nextBoundary = m_data.worldMin[i] + (cell[i] + 1) * m_data.cellSize[i];
            tMax[i] = (nextBoundary - ray.origin[i]) / safeDir[i];
        }
        else {
            step[i] = -1;
            float nextBoundary = m_data.worldMin[i] + cell[i] * m_data.cellSize[i];
            tMax[i] = (nextBoundary - ray.origin[i]) / safeDir[i];
        }
        tDelta[i] = m_data.cellSize[i] / std::abs(safeDir[i]);
    }

    int maxSteps = 200;
    int steps = 0;
    float currentT = 0;

    while (steps < maxSteps && currentT < maxDistance) {
        if (m_data.isValidCell(cell.x, cell.y, cell.z)) {
            int idx = m_data.getCellIndex(cell.x, cell.y, cell.z);
            const auto& gridCell = m_data.cells[idx];

            // Добавляем уникальные индексы
            for (int val : gridCell.snakeIndices) {
                if (std::find(snakeIndices.begin(), snakeIndices.end(), val) == snakeIndices.end()) {
                    snakeIndices.push_back(val);
                }
            }
            for (int val : gridCell.foodIndices) {
                if (std::find(foodIndices.begin(), foodIndices.end(), val) == foodIndices.end()) {
                    foodIndices.push_back(val);
                }
            }
            for (int val : gridCell.obstacleIndices) {
                if (std::find(obstacleIndices.begin(), obstacleIndices.end(), val) == obstacleIndices.end()) {
                    obstacleIndices.push_back(val);
                }
            }
            for (int val : gridCell.fenceIndices) {
                if (std::find(fenceIndices.begin(), fenceIndices.end(), val) == fenceIndices.end()) {
                    fenceIndices.push_back(val);
                }
            }
            for (int val : gridCell.birdIndices) {
                if (std::find(birdIndices.begin(), birdIndices.end(), val) == birdIndices.end()) {
                    birdIndices.push_back(val);
                }
            }
            for (int val : gridCell.cloudIndices) {
                if (std::find(cloudIndices.begin(), cloudIndices.end(), val) == cloudIndices.end()) {
                    cloudIndices.push_back(val);
                }
            }
            for (int val : gridCell.flowerIndices) {
                if (std::find(flowerIndices.begin(), flowerIndices.end(), val) == flowerIndices.end()) {
                    flowerIndices.push_back(val);
                }
            }
        }

        // Переход к следующей ячейке
        if (tMax.x < tMax.y && tMax.x < tMax.z) {
            currentT = tMax.x;
            cell.x += step.x;
            tMax.x += tDelta.x;
        }
        else if (tMax.y < tMax.x && tMax.y < tMax.z) {
            currentT = tMax.y;
            cell.y += step.y;
            tMax.y += tDelta.y;
        }
        else {
            currentT = tMax.z;
            cell.z += step.z;
            tMax.z += tDelta.z;
        }
        steps++;
    }
}

size_t GridAccelerator::getNonEmptyCells() const {
    size_t count = 0;
    for (const auto& cell : m_data.cells) {
        if (cell.hasAny()) count++;
    }
    return count;
}

float GridAccelerator::getFillRatio() const {
    if (m_data.cells.empty()) return 0.0f;
    return (float)getNonEmptyCells() / m_data.cells.size() * 100.0f;
}

void GridAccelerator::printStats() const {
    if (!m_enabled) {
        std::cout << "GridAccelerator: DISABLED" << std::endl;
        return;
    }

    std::cout << "=== GridAccelerator Stats ===" << std::endl;
    std::cout << "Grid size: " << m_data.gridSize.x << "x"
        << m_data.gridSize.y << "x" << m_data.gridSize.z << std::endl;
    std::cout << "Total cells: " << getTotalCells() << std::endl;
    std::cout << "Non-empty cells: " << getNonEmptyCells() << std::endl;
    std::cout << "Fill ratio: " << getFillRatio() << "%" << std::endl;
    std::cout << "Total objects: " << m_totalObjects << std::endl;
    std::cout << "Avg objects per non-empty cell: "
        << (getNonEmptyCells() > 0 ? (float)m_totalObjects / getNonEmptyCells() : 0.0f) << std::endl;
    std::cout << "============================" << std::endl;
}