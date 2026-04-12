#include "../pch.h"
#include "ShadowMapper.h"
#include <algorithm>
#include <cmath>
#include <chrono>

ShadowMapper::ShadowMapper()
    : m_gridWidth(0)
    , m_gridDepth(0)
    , m_cellSize(0.1f)
    , m_groundHeight(0.0f)
    , m_totalCellsX(0)
    , m_totalCellsZ(0)
    , m_lightType(LightType::Directional)
    , m_lightPos(0.0f, 5.0f, 0.0f)
    , m_lightDirection(1.0f, 0.2f, 0.5f)
    , m_lightColor(0.9f, 0.85f, 0.75f)
    , m_recordDebugRays(false)
    , m_nextRayId(0)
{
    m_lightDirection = glm::normalize(m_lightDirection);
    setGrid(0, 0, 0.1f, 0.0f);
}

ShadowMapper::ShadowMapper(int width, int depth, float cellSize, float groundHeight,
    const glm::vec3& lightDirection, const glm::vec3& lightColor,
    LightType lightType, const glm::vec3& lightPos,
    int strideX, int strideZ)
    : m_gridWidth(width)
    , m_gridDepth(depth)
    , m_cellSize(cellSize)
    , m_groundHeight(groundHeight)
    , m_lightType(lightType)
    , m_lightPos(lightPos)
    , m_lightDirection(glm::normalize(lightDirection))
    , m_lightColor(lightColor)
    , m_recordDebugRays(false)
    , m_nextRayId(0)
{
    setGrid(width, depth, cellSize, groundHeight);
}

void ShadowMapper::setGrid(int width, int depth, float cellSize, float groundHeight)
{
    m_gridWidth = width;
    m_gridDepth = depth;
    m_cellSize = cellSize;
    m_groundHeight = groundHeight;

    m_totalCellsX = m_gridWidth;
    m_totalCellsZ = m_gridDepth;

    std::cout << "=== ShadowMapper Grid Setup ===" << std::endl;
    std::cout << "Game grid cells: " << m_totalCellsX << " x " << m_totalCellsZ << std::endl;
    std::cout << "Total cells: " << (m_totalCellsX * m_totalCellsZ) << std::endl;
    std::cout << "===============================" << std::endl;

    // Инициализируем сетку теней
    m_shadowGrid.resize(m_totalCellsZ, std::vector<ShadowSample>(m_totalCellsX));

    for (int z = 0; z < m_totalCellsZ; z++) {
        for (int x = 0; x < m_totalCellsX; x++) {
            m_shadowGrid[z][x].cellX = x;
            m_shadowGrid[z][x].cellZ = z;
            m_shadowGrid[z][x].position = getCellCenter(x, z);
            m_shadowGrid[z][x].computed = false;
            m_shadowGrid[z][x].value = 1.0f; // По умолчанию на свету
        }
    }
}

glm::vec3 ShadowMapper::getCellCenter(int cellX, int cellZ) const
{
    float halfWidth = m_gridWidth * m_cellSize / 2.0f;
    float halfDepth = m_gridDepth * m_cellSize / 2.0f;

    float worldX = cellX * m_cellSize - halfWidth + (m_cellSize / 2.0f);
    float worldZ = cellZ * m_cellSize - halfDepth + (m_cellSize / 2.0f);

    return glm::vec3(worldX, m_groundHeight + 0.05f, worldZ);
}

void ShadowMapper::setLightDirection(const glm::vec3& direction)
{
    m_lightDirection = glm::normalize(direction);
}

void ShadowMapper::setLightColor(const glm::vec3& color)
{
    m_lightColor = color;
}

void ShadowMapper::registerObjectBounds(const std::vector<BoundingSphere>& spheres)
{
    m_objectSpheres = spheres;
}

void ShadowMapper::clearObjectBounds()
{
    m_objectSpheres.clear();
}

void ShadowMapper::setIntersectCallback(std::function<bool(const struct Ray&, float&, glm::vec3&)> callback)
{
    m_intersectCallback = callback;
}

bool ShadowMapper::intersectsAnyObject(const struct Ray& ray, float& hitDistance, glm::vec3& hitPoint)
{
    float closestHit = 1000.0f;
    glm::vec3 closestPoint;
    bool hit = false;

    // Проверка через коллбек (для точных моделей)
    if (m_intersectCallback) {
        float exactHitDistance;
        glm::vec3 exactHitPoint;
        if (m_intersectCallback(ray, exactHitDistance, exactHitPoint)) {
            if (exactHitDistance > 0.01f && exactHitDistance < closestHit) {
                closestHit = exactHitDistance;
                closestPoint = exactHitPoint;
                hit = true;
            }
        }
    }

    // Проверка через bounding spheres (быстрая)
    for (const auto& sphere : m_objectSpheres) {
        glm::vec3 oc = ray.origin - sphere.center;
        float a = glm::dot(ray.direction, ray.direction);
        float b = 2.0f * glm::dot(oc, ray.direction);
        float c = glm::dot(oc, oc) - sphere.radius * sphere.radius;
        float discriminant = b * b - 4 * a * c;

        if (discriminant >= 0) {
            float sqrtD = sqrt(discriminant);
            float t1 = (-b - sqrtD) / (2.0f * a);
            float t2 = (-b + sqrtD) / (2.0f * a);

            if (t1 > 0.01f && t1 < closestHit) {
                closestHit = t1;
                closestPoint = ray.pointAt(t1);
                hit = true;
            }
            if (t2 > 0.01f && t2 < closestHit) {
                closestHit = t2;
                closestPoint = ray.pointAt(t2);
                hit = true;
            }
        }
    }

    if (hit) {
        hitDistance = closestHit;
        hitPoint = closestPoint;
    }
    return hit;
}

bool ShadowMapper::traceShadowRay(const glm::vec3& start, const glm::vec3& direction,
    float maxDistance, float& hitDistance, glm::vec3& hitPoint)
{
    Ray shadowRay(start, direction);
    return intersectsAnyObject(shadowRay, hitDistance, hitPoint);
}

bool ShadowMapper::isPointInShadow(const glm::vec3& point, int& hitCellX, int& hitCellZ, float& hitDistance)
{
    glm::vec3 rayOrigin;
    glm::vec3 rayDirection;
    float maxRayDistance = 100.0f;
    glm::vec3 hitPoint;

    switch (m_lightType) {
    case LightType::Directional:
        rayOrigin = point;
        rayDirection = -m_lightDirection;
        maxRayDistance = 100.0f;
        break;

    case LightType::Points:
        // Точечный свет: луч от точки к источнику света
        rayOrigin = point;
        rayDirection = glm::normalize(m_lightPos - point);
        maxRayDistance = glm::length(m_lightPos - point);
        break;

    case LightType::Spot:
        rayOrigin = m_lightPos;
        rayDirection = glm::normalize(point - m_lightPos);
        maxRayDistance = glm::length(point - m_lightPos);

        glm::vec3 lightDirNormalized = glm::normalize(m_lightDirection);
        float cosAngle = glm::dot(rayDirection, lightDirNormalized);
        float spotAngle = cos(glm::radians(45.0f));

        if (cosAngle < spotAngle) {
            hitDistance = maxRayDistance;
            hitCellX = -1;
            hitCellZ = -1;
            return true;
        }
        break;
    }

    float hitDist;
    glm::vec3 hitPt;
    bool hit = traceShadowRay(rayOrigin, rayDirection, maxRayDistance, hitDist, hitPt);

    if (hit) {
        hitDistance = hitDist;
        float halfWidth = m_gridWidth * m_cellSize / 2.0f;
        float halfDepth = m_gridDepth * m_cellSize / 2.0f;

        hitCellX = (int)((hitPt.x + halfWidth) / m_cellSize);
        hitCellZ = (int)((hitPt.z + halfDepth) / m_cellSize);

        hitCellX = std::max(0, std::min(hitCellX, m_totalCellsX - 1));
        hitCellZ = std::max(0, std::min(hitCellZ, m_totalCellsZ - 1));
    }

    return hit;
}
void ShadowMapper::recordRay(const glm::vec3& origin, const glm::vec3& direction,
    const glm::vec3& hitPoint, float distance, bool hit, int cellX, int cellZ)
{
    if (!m_recordDebugRays) return;

    DebugRay ray;
    ray.origin = origin;
    ray.direction = glm::normalize(direction);
    ray.hitPoint = hitPoint;
    ray.distance = distance;
    ray.hit = hit;
    ray.rayId = m_nextRayId++;
    ray.cellX = cellX;
    ray.cellZ = cellZ;

    m_debugRays.push_back(ray);
}

void ShadowMapper::computeShadows()
{
    if (m_totalCellsX == 0 || m_totalCellsZ == 0) return;

    auto startTime = std::chrono::high_resolution_clock::now();

    if (m_recordDebugRays) {
        clearDebugRays();
        std::cout << "Recording debug rays for shadow computation..." << std::endl;
    }

    std::cout << "\n=== SHADOW MAPPER: Computing shadows ===" << std::endl;
    std::cout << "Grid cells: " << m_totalCellsX << " x " << m_totalCellsZ << std::endl;
    std::cout << "Total cells: " << (m_totalCellsX * m_totalCellsZ) << std::endl;

    if (m_recordDebugRays) {
        std::cout << "DEBUG RAYS ENABLED - recording all shadow rays!" << std::endl;
    }

    int shadowCount = 0;
    int totalCells = m_totalCellsX * m_totalCellsZ;

    // Для каждой клетки пускаем луч
    for (int z = 0; z < m_totalCellsZ; z++) {
        for (int x = 0; x < m_totalCellsX; x++) {
            ShadowSample& sample = m_shadowGrid[z][x];

            int hitCellX, hitCellZ;
            float hitDistance;

            bool inShadow = isPointInShadow(sample.position, hitCellX, hitCellZ, hitDistance);

            if (inShadow) {
                sample.value = 0.0f;  // В тени
                shadowCount++;
            }
            else {
                sample.value = 1.0f;  // На свету
            }

            sample.computed = true;

            // Запись луча для отладки (если включено)
            if (m_recordDebugRays) {
                glm::vec3 rayOrigin, rayDirection, endPoint;
                float maxDist;

                if (m_lightType == LightType::Directional) {
                    rayOrigin = sample.position;
                    rayDirection = -m_lightDirection;
                    maxDist = 50.0f;
                    endPoint = rayOrigin + rayDirection * maxDist;
                }
                else {
                    rayOrigin = sample.position;
                    rayDirection = glm::normalize(m_lightPos - sample.position);
                    maxDist = glm::length(m_lightPos - sample.position);
                    endPoint = m_lightPos;
                }

                // Если есть попадание, используем точку попадания
                if (inShadow && hitCellX >= 0 && hitCellZ >= 0) {
                    endPoint = getCellCenter(hitCellX, hitCellZ);
                    endPoint.y += 0.1f;
                }

                recordRay(rayOrigin, rayDirection, endPoint, maxDist, inShadow, x, z);
            }
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

    float shadowPercent = (float)shadowCount / totalCells * 100.0f;

    std::cout << "Cells in shadow: " << shadowCount << " / " << totalCells
        << " (" << shadowPercent << "%)" << std::endl;
    std::cout << "Time: " << durationMs << " ms" << std::endl;

    if (m_recordDebugRays) {
        std::cout << "Debug rays recorded: " << m_debugRays.size() << std::endl;
    }

    std::cout << "=========================================\n" << std::endl;
}

float ShadowMapper::getShadowAtCell(int cellX, int cellZ) const
{
    if (cellX < 0 || cellX >= m_totalCellsX || cellZ < 0 || cellZ >= m_totalCellsZ) {
        return 1.0f;
    }
    return m_shadowGrid[cellZ][cellX].value;
}

float ShadowMapper::getShadowAtPoint(const glm::vec3& point) const
{
    // Определяем клетку по точке
    float halfWidth = m_gridWidth * m_cellSize / 2.0f;
    float halfDepth = m_gridDepth * m_cellSize / 2.0f;

    int cellX = (int)((point.x + halfWidth) / m_cellSize);
    int cellZ = (int)((point.z + halfDepth) / m_cellSize);

    cellX = std::max(0, std::min(cellX, m_totalCellsX - 1));
    cellZ = std::max(0, std::min(cellZ, m_totalCellsZ - 1));

    return m_shadowGrid[cellZ][cellX].value;
}

float ShadowMapper::getShadowAtWorldPos(float x, float z) const
{
    return getShadowAtPoint(glm::vec3(x, 0.0f, z));
}

bool ShadowMapper::isInGridBounds(int x, int z) const
{
    return x >= 0 && x < m_totalCellsX && z >= 0 && z < m_totalCellsZ;
}