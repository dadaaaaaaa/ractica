// ShadowMapper.cpp
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
    , m_strideX(1)
    , m_strideZ(1)
    , m_lightType(LightType::Directional)
    , m_lightPos(0.0f, 5.0f, 0.0f)
    , m_lightDirection(1.0f, 0.2f, 0.5f)
    , m_lightColor(0.9f, 0.85f, 0.75f)
    , m_shadowTraceMode(TRACE_CENTER)
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
    , m_strideX(strideX > 0 ? strideX : 1)
    , m_strideZ(strideZ > 0 ? strideZ : 1)
    , m_lightType(lightType)
    , m_lightPos(lightPos)
    , m_lightDirection(glm::normalize(lightDirection))
    , m_lightColor(lightColor)
    , m_shadowTraceMode(TRACE_CENTER)
    , m_recordDebugRays(false)
    , m_nextRayId(0)
{
    setGrid(width, depth, cellSize, groundHeight);
}

const char* ShadowMapper::getModeName(ShadowTraceMode mode) {
    switch (mode) {
    case TRACE_CENTER: return "CENTER (1 ray per cell, binary)";
    case TRACE_CORNERS: return "CORNERS (4 rays per cell + gradient)";
    case TRACE_CENTER_SUBDIVIDED: return "CENTER_SUBDIVIDED (adaptive: check center, then 100 rays, binary per subcell)";
    case TRACE_CORNERS_SUBDIVIDED: return "CORNERS_SUBDIVIDED (adaptive: check center, then 400 rays, gradient per subcell)";
    default: return "UNKNOWN";
    }
}

std::vector<DebugRay> ShadowMapper::getRaysByType(DebugRay::RayType type) const {
    std::vector<DebugRay> result;
    for (const auto& ray : m_debugRays) {
        if (ray.rayType == type) {
            result.push_back(ray);
        }
    }
    return result;
}

std::vector<DebugRay> ShadowMapper::getHitRays() const {
    std::vector<DebugRay> result;
    for (const auto& ray : m_debugRays) {
        if (ray.hit) {
            result.push_back(ray);
        }
    }
    return result;
}

std::vector<DebugRay> ShadowMapper::getMissRays() const {
    std::vector<DebugRay> result;
    for (const auto& ray : m_debugRays) {
        if (!ray.hit) {
            result.push_back(ray);
        }
    }
    return result;
}

void ShadowMapper::setShadowTraceMode(ShadowTraceMode mode)
{
    if (m_shadowTraceMode == mode) return;

    m_shadowTraceMode = mode;

    // Обновляем useGradient для всех клеток
    bool useGradient = (mode == TRACE_CORNERS || mode == TRACE_CORNERS_SUBDIVIDED);

    for (int z = 0; z < m_totalCellsZ; z++) {
        for (int x = 0; x < m_totalCellsX; x++) {
            m_shadowGrid[z][x].useGradient = useGradient;
        }
    }

    std::cout << "Shadow trace mode changed to: " << getModeName(mode) << std::endl;
}

void ShadowMapper::initAllSubCells()
{
    for (int z = 0; z < m_totalCellsZ; z++) {
        for (int x = 0; x < m_totalCellsX; x++) {
            if (m_shadowGrid[z][x].subCellValues.empty()) {
                m_shadowGrid[z][x].subCellValues.resize(SHADOW_SUBDIVISION_SIZE,
                    std::vector<float>(SHADOW_SUBDIVISION_SIZE, 1.0f));
            }
        }
    }
}

void ShadowMapper::setGrid(int width, int depth, float cellSize, float groundHeight)
{
    m_gridWidth = width;
    m_gridDepth = depth;
    m_cellSize = cellSize;
    m_groundHeight = groundHeight;

    // Вычисляем реальный размер сетки теней с учётом stride
    m_totalCellsX = m_gridWidth / m_strideX;
    m_totalCellsZ = m_gridDepth / m_strideZ;

    if (m_totalCellsX < 1) m_totalCellsX = 1;
    if (m_totalCellsZ < 1) m_totalCellsZ = 1;

    float shadowCellSize = m_cellSize * m_strideX;

    std::cout << "=== ShadowMapper Grid Setup ===" << std::endl;
    std::cout << "Original grid cells: " << m_gridWidth << " x " << m_gridDepth << std::endl;
    std::cout << "Stride: " << m_strideX << " x " << m_strideZ << std::endl;
    std::cout << "Shadow grid cells: " << m_totalCellsX << " x " << m_totalCellsZ << std::endl;
    std::cout << "Shadow cell size: " << shadowCellSize << std::endl;
    std::cout << "Total shadow cells: " << (m_totalCellsX * m_totalCellsZ) << std::endl;
    std::cout << "Trace mode: " << getModeName(m_shadowTraceMode) << std::endl;
    std::cout << "===============================" << std::endl;

    // Инициализируем сетку теней
    m_shadowGrid.resize(m_totalCellsZ, std::vector<ShadowSample>(m_totalCellsX));

    bool useGradient = (m_shadowTraceMode == TRACE_CORNERS ||
        m_shadowTraceMode == TRACE_CORNERS_SUBDIVIDED);

    for (int z = 0; z < m_totalCellsZ; z++) {
        for (int x = 0; x < m_totalCellsX; x++) {
            m_shadowGrid[z][x].cellX = x;
            m_shadowGrid[z][x].cellZ = z;
            m_shadowGrid[z][x].useGradient = useGradient;

            // Позиция центра клетки в оригинальной сетке
            int origX = x * m_strideX + m_strideX / 2;
            int origZ = z * m_strideZ + m_strideZ / 2;
            m_shadowGrid[z][x].position = getCellCenter(origX, origZ);
            m_shadowGrid[z][x].computed = false;
            m_shadowGrid[z][x].value = 1.0f;
            m_shadowGrid[z][x].centerWasLit = false;
            for (int i = 0; i < 4; i++) {
                m_shadowGrid[z][x].cornerShadows[i] = 1.0f;
            }
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

glm::vec3 ShadowMapper::getCornerWorldPosition(int cellX, int cellZ, int cornerIndex) const
{
    float halfWidth = m_gridWidth * m_cellSize / 2.0f;
    float halfDepth = m_gridDepth * m_cellSize / 2.0f;

    int origCellX = cellX * m_strideX;
    int origCellZ = cellZ * m_strideZ;

    float worldX = origCellX * m_cellSize - halfWidth;
    float worldZ = origCellZ * m_cellSize - halfDepth;

    float cellWidthX = m_cellSize * m_strideX;
    float cellWidthZ = m_cellSize * m_strideZ;

    switch (cornerIndex) {
    case 0:
        return glm::vec3(worldX, m_groundHeight + 0.05f, worldZ);
    case 1:
        return glm::vec3(worldX + cellWidthX, m_groundHeight + 0.05f, worldZ);
    case 2:
        return glm::vec3(worldX, m_groundHeight + 0.05f, worldZ + cellWidthZ);
    case 3:
        return glm::vec3(worldX + cellWidthX, m_groundHeight + 0.05f, worldZ + cellWidthZ);
    default:
        return glm::vec3(worldX, m_groundHeight + 0.05f, worldZ);
    }
}

glm::vec3 ShadowMapper::getSubCellCenter(int cellX, int cellZ, int subX, int subZ) const
{
    float halfWidth = m_gridWidth * m_cellSize / 2.0f;
    float halfDepth = m_gridDepth * m_cellSize / 2.0f;

    int origStartX = cellX * m_strideX;
    int origStartZ = cellZ * m_strideZ;

    float subCellSizeX = (float)m_strideX / SHADOW_SUBDIVISION_SIZE;
    float subCellSizeZ = (float)m_strideZ / SHADOW_SUBDIVISION_SIZE;

    float subCenterX = origStartX + (subX + 0.5f) * subCellSizeX;
    float subCenterZ = origStartZ + (subZ + 0.5f) * subCellSizeZ;

    float worldX = subCenterX * m_cellSize - halfWidth;
    float worldZ = subCenterZ * m_cellSize - halfDepth;

    return glm::vec3(worldX, m_groundHeight + 0.05f, worldZ);
}

glm::vec3 ShadowMapper::getSubCellCorner(int cellX, int cellZ, int subX, int subZ, int cornerIndex) const
{
    float halfWidth = m_gridWidth * m_cellSize / 2.0f;
    float halfDepth = m_gridDepth * m_cellSize / 2.0f;

    int origStartX = cellX * m_strideX;
    int origStartZ = cellZ * m_strideZ;

    float subCellSizeX = (float)m_strideX / SHADOW_SUBDIVISION_SIZE;
    float subCellSizeZ = (float)m_strideZ / SHADOW_SUBDIVISION_SIZE;

    float subBaseX = origStartX + subX * subCellSizeX;
    float subBaseZ = origStartZ + subZ * subCellSizeZ;

    float worldX, worldZ;

    switch (cornerIndex) {
    case 0:
        worldX = subBaseX * m_cellSize - halfWidth;
        worldZ = subBaseZ * m_cellSize - halfDepth;
        break;
    case 1:
        worldX = (subBaseX + subCellSizeX) * m_cellSize - halfWidth;
        worldZ = subBaseZ * m_cellSize - halfDepth;
        break;
    case 2:
        worldX = subBaseX * m_cellSize - halfWidth;
        worldZ = (subBaseZ + subCellSizeZ) * m_cellSize - halfDepth;
        break;
    case 3:
        worldX = (subBaseX + subCellSizeX) * m_cellSize - halfWidth;
        worldZ = (subBaseZ + subCellSizeZ) * m_cellSize - halfDepth;
        break;
    default:
        worldX = subBaseX * m_cellSize - halfWidth;
        worldZ = subBaseZ * m_cellSize - halfDepth;
    }

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

            if (t1 > 0.05f && t1 < closestHit) {
                closestHit = t1;
                closestPoint = ray.pointAt(t1);
                hit = true;
            }
            if (t2 > 0.05f && t2 < closestHit) {
                closestHit = t2;
                closestPoint = ray.pointAt(t2);
                hit = true;
            }
        }
    }

    if (m_intersectCallback) {
        float exactHitDistance;
        glm::vec3 exactHitPoint;
        if (m_intersectCallback(ray, exactHitDistance, exactHitPoint)) {
            if (exactHitDistance > 0.05f && exactHitDistance < closestHit) {
                closestHit = exactHitDistance;
                closestPoint = exactHitPoint;
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
        // Луч ОТ источника света (далеко за сценой) К точке на земле
        rayOrigin = point - m_lightDirection * 100.0f;
        rayDirection = m_lightDirection;
        maxRayDistance = 100.0f;
        break;

    case LightType::Points:
    {
        glm::vec3 toPoint = point - m_lightPos;
        float distanceToPoint = glm::length(toPoint);

        if (distanceToPoint < 0.01f) {
            hitCellX = -1;
            hitCellZ = -1;
            hitDistance = 0;
            return false;
        }

        rayOrigin = m_lightPos;
        rayDirection = glm::normalize(toPoint);
        maxRayDistance = distanceToPoint;

        float epsilon = 0.05f;
        rayOrigin += rayDirection * epsilon;
        maxRayDistance -= epsilon;
    }
    break;

    case LightType::Spot:
    {
        glm::vec3 toPoint = point - m_lightPos;
        float distanceToPoint = glm::length(toPoint);

        if (distanceToPoint < 0.01f) {
            hitCellX = -1;
            hitCellZ = -1;
            hitDistance = 0;
            return false;
        }

        glm::vec3 toPointDir = glm::normalize(toPoint);
        glm::vec3 lightDirNormalized = glm::normalize(m_lightDirection);

        float cosAngle = glm::dot(toPointDir, lightDirNormalized);
        float spotCos = cos(glm::radians(45.0f));

        if (cosAngle < spotCos) {
            hitDistance = distanceToPoint;
            hitCellX = -1;
            hitCellZ = -1;
            return true;
        }

        rayOrigin = m_lightPos;
        rayDirection = toPointDir;
        maxRayDistance = distanceToPoint;

        float epsilon = 0.05f;
        rayOrigin += rayDirection * epsilon;
        maxRayDistance -= epsilon;
    }
    break;
    }

    float hitDist;
    glm::vec3 hitPt;
    bool hit = traceShadowRay(rayOrigin, rayDirection, maxRayDistance, hitDist, hitPt);

    if (hit && hitDist < maxRayDistance - 0.01f) {
        hitDistance = hitDist;
        float halfWidth = m_gridWidth * m_cellSize / 2.0f;
        float halfDepth = m_gridDepth * m_cellSize / 2.0f;

        hitCellX = (int)((hitPt.x + halfWidth) / m_cellSize);
        hitCellZ = (int)((hitPt.z + halfDepth) / m_cellSize);

        hitCellX = std::max(0, std::min(hitCellX, m_gridWidth - 1));
        hitCellZ = std::max(0, std::min(hitCellZ, m_gridDepth - 1));

        return true;
    }

    hitCellX = -1;
    hitCellZ = -1;
    hitDistance = maxRayDistance;
    return false;
}

float ShadowMapper::computeCornerShadow(const glm::vec3& cornerPos, int& hitCellX, int& hitCellZ, float& hitDistance)
{
    bool inShadow = isPointInShadow(cornerPos, hitCellX, hitCellZ, hitDistance);
    return inShadow ? 0.0f : 1.0f;
}

void ShadowMapper::computeCellGradient(ShadowSample& sample, int cellX, int cellZ)
{
    int hitCount = 0;

    for (int corner = 0; corner < 4; corner++) {
        glm::vec3 cornerPos = getCornerWorldPosition(cellX, cellZ, corner);
        int hitCellX, hitCellZ;
        float hitDistance;

        bool inShadow = isPointInShadow(cornerPos, hitCellX, hitCellZ, hitDistance);

        sample.cornerShadows[corner] = inShadow ? 0.0f : 1.0f;

        if (!inShadow) {
            hitCount++;
        }

        if (m_recordDebugRays) {
            glm::vec3 rayOrigin, rayDirection, endPoint;

            if (m_lightType == LightType::Directional) {
                rayOrigin = cornerPos - m_lightDirection * 50.0f;
                rayDirection = m_lightDirection;
                endPoint = cornerPos;
            }
            else {
                rayOrigin = m_lightPos;
                rayDirection = glm::normalize(cornerPos - m_lightPos);
                endPoint = cornerPos;
            }

            if (inShadow && hitCellX >= 0 && hitCellZ >= 0) {
                endPoint = getCellCenter(hitCellX, hitCellZ);
                endPoint.y += 0.1f;
            }

            recordRay(rayOrigin, rayDirection, endPoint, 50.0f, inShadow, cellX, cellZ, corner, -1, -1,
                false, false, DebugRay::RAY_CORNER);
        }
    }

    sample.value = (float)hitCount / 4.0f;
}

void ShadowMapper::computeCellCenterSubdivided(ShadowSample& sample, int cellX, int cellZ)
{
    // Сначала проверяем центр клетки
    int origCenterX = cellX * m_strideX + m_strideX / 2;
    int origCenterZ = cellZ * m_strideZ + m_strideZ / 2;
    glm::vec3 centerPos = getCellCenter(origCenterX, origCenterZ);

    int hitCellX, hitCellZ;
    float hitDistance;
    bool centerInShadow = isPointInShadow(centerPos, hitCellX, hitCellZ, hitDistance);

    // Записываем центральный луч для отладки
    if (m_recordDebugRays) {
        glm::vec3 rayOrigin, rayDirection, endPoint;
        if (m_lightType == LightType::Directional) {
            rayOrigin = centerPos - m_lightDirection * 50.0f;
            rayDirection = m_lightDirection;
            endPoint = centerPos;
        }
        else {
            rayOrigin = m_lightPos;
            rayDirection = glm::normalize(centerPos - m_lightPos);
            endPoint = centerPos;
        }
        if (centerInShadow && hitCellX >= 0 && hitCellZ >= 0) {
            endPoint = getCellCenter(hitCellX, hitCellZ);
            endPoint.y += 0.1f;
        }
        recordRay(rayOrigin, rayDirection, endPoint, 50.0f, centerInShadow, cellX, cellZ, -1, -1, -1,
            false, false, DebugRay::RAY_CENTER);
    }

    // Если центр НЕ в тени - вся клетка освещена (все подклетки = 1.0)
    if (!centerInShadow) {
        sample.value = 1.0f;
        sample.centerWasLit = true;

        sample.initSubCells();
        for (int subZ = 0; subZ < SHADOW_SUBDIVISION_SIZE; subZ++) {
            for (int subX = 0; subX < SHADOW_SUBDIVISION_SIZE; subX++) {
                sample.subCellValues[subZ][subX] = 1.0f;
            }
        }
        return;
    }

    sample.centerWasLit = false;
    sample.initSubCells();

    int totalSubCells = SHADOW_SUBDIVISION_SIZE * SHADOW_SUBDIVISION_SIZE;
    float sum = 0.0f;

    for (int subZ = 0; subZ < SHADOW_SUBDIVISION_SIZE; subZ++) {
        for (int subX = 0; subX < SHADOW_SUBDIVISION_SIZE; subX++) {
            glm::vec3 subCenter = getSubCellCenter(cellX, cellZ, subX, subZ);
            int subHitCellX, subHitCellZ;
            float subHitDistance;

            bool inShadow = isPointInShadow(subCenter, subHitCellX, subHitCellZ, subHitDistance);

            // КАЖДАЯ ПОДКЛЕТКА ПОЛУЧАЕТ СВОЁ ЗНАЧЕНИЕ (0 или 1)
            float subValue = inShadow ? 0.0f : 1.0f;
            sample.subCellValues[subZ][subX] = subValue;
            sum += subValue;

            if (m_recordDebugRays) {
                glm::vec3 rayOrigin, rayDirection, endPoint;
                if (m_lightType == LightType::Directional) {
                    rayOrigin = subCenter - m_lightDirection * 50.0f;
                    rayDirection = m_lightDirection;
                    endPoint = subCenter;
                }
                else {
                    rayOrigin = m_lightPos;
                    rayDirection = glm::normalize(subCenter - m_lightPos);
                    endPoint = subCenter;
                }
                if (inShadow && subHitCellX >= 0 && subHitCellZ >= 0) {
                    endPoint = getCellCenter(subHitCellX, subHitCellZ);
                    endPoint.y += 0.1f;
                }
                recordRay(rayOrigin, rayDirection, endPoint, 50.0f, inShadow, cellX, cellZ, -1, subX, subZ,
                    true, false, DebugRay::RAY_SUB_CENTER);
            }
        }
    }

    sample.value = sum / totalSubCells;
}

void ShadowMapper::computeCellCornersSubdivided(ShadowSample& sample, int cellX, int cellZ)
{
    // Сначала проверяем центр клетки
    int origCenterX = cellX * m_strideX + m_strideX / 2;
    int origCenterZ = cellZ * m_strideZ + m_strideZ / 2;
    glm::vec3 centerPos = getCellCenter(origCenterX, origCenterZ);

    int hitCellX, hitCellZ;
    float hitDistance;
    bool centerInShadow = isPointInShadow(centerPos, hitCellX, hitCellZ, hitDistance);

    // Записываем центральный луч для отладки
    if (m_recordDebugRays) {
        glm::vec3 rayOrigin, rayDirection, endPoint;
        if (m_lightType == LightType::Directional) {
            rayOrigin = centerPos - m_lightDirection * 50.0f;
            rayDirection = m_lightDirection;
            endPoint = centerPos;
        }
        else {
            rayOrigin = m_lightPos;
            rayDirection = glm::normalize(centerPos - m_lightPos);
            endPoint = centerPos;
        }
        if (centerInShadow && hitCellX >= 0 && hitCellZ >= 0) {
            endPoint = getCellCenter(hitCellX, hitCellZ);
            endPoint.y += 0.1f;
        }
        recordRay(rayOrigin, rayDirection, endPoint, 50.0f, centerInShadow, cellX, cellZ, -1, -1, -1,
            false, false, DebugRay::RAY_CENTER);
    }

    // Если центр НЕ в тени - вся клетка освещена (все подклетки = 1.0)
    if (!centerInShadow) {
        sample.value = 1.0f;
        sample.centerWasLit = true;

        sample.initSubCells();
        for (int subZ = 0; subZ < SHADOW_SUBDIVISION_SIZE; subZ++) {
            for (int subX = 0; subX < SHADOW_SUBDIVISION_SIZE; subX++) {
                sample.subCellValues[subZ][subX] = 1.0f;
            }
        }
        return;
    }

    sample.centerWasLit = false;
    sample.initSubCells();

    int totalSubCells = SHADOW_SUBDIVISION_SIZE * SHADOW_SUBDIVISION_SIZE;
    float sum = 0.0f;

    for (int subZ = 0; subZ < SHADOW_SUBDIVISION_SIZE; subZ++) {
        for (int subX = 0; subX < SHADOW_SUBDIVISION_SIZE; subX++) {
            int litCorners = 0;

            for (int corner = 0; corner < 4; corner++) {
                glm::vec3 cornerPos = getSubCellCorner(cellX, cellZ, subX, subZ, corner);
                int subHitCellX, subHitCellZ;
                float subHitDistance;

                bool inShadow = isPointInShadow(cornerPos, subHitCellX, subHitCellZ, subHitDistance);

                if (!inShadow) {
                    litCorners++;
                }

                if (m_recordDebugRays) {
                    glm::vec3 rayOrigin, rayDirection, endPoint;
                    if (m_lightType == LightType::Directional) {
                        rayOrigin = cornerPos - m_lightDirection * 50.0f;
                        rayDirection = m_lightDirection;
                        endPoint = cornerPos;
                    }
                    else {
                        rayOrigin = m_lightPos;
                        rayDirection = glm::normalize(cornerPos - m_lightPos);
                        endPoint = cornerPos;
                    }
                    if (inShadow && subHitCellX >= 0 && subHitCellZ >= 0) {
                        endPoint = getCellCenter(subHitCellX, subHitCellZ);
                        endPoint.y += 0.1f;
                    }
                    recordRay(rayOrigin, rayDirection, endPoint, 50.0f, inShadow, cellX, cellZ, corner, subX, subZ,
                        true, false, DebugRay::RAY_SUB_CORNER);
                }
            }

            // КАЖДАЯ ПОДКЛЕТКА ПОЛУЧАЕТ СВОЁ ЗНАЧЕНИЕ (градиент 0..1)
            float subCellValue = (float)litCorners / 4.0f;
            sample.subCellValues[subZ][subX] = subCellValue;
            sum += subCellValue;
        }
    }

    sample.value = sum / totalSubCells;
}

void ShadowMapper::recordRay(const glm::vec3& origin, const glm::vec3& direction,
    const glm::vec3& hitPoint, float distance, bool hit,
    int cellX, int cellZ, int cornerIndex, int subCellX, int subCellZ,
    bool isSubdivided, bool centerCheckPassed, DebugRay::RayType rayType)
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
    ray.cornerIndex = cornerIndex;
    ray.subCellX = subCellX;
    ray.subCellZ = subCellZ;
    ray.isSubdivided = isSubdivided;
    ray.centerCheckPassed = centerCheckPassed;
    ray.rayType = rayType;

    m_debugRays.push_back(ray);
}

void ShadowMapper::computeShadows()
{
    if (m_totalCellsX == 0 || m_totalCellsZ == 0) return;

    auto startTime = std::chrono::high_resolution_clock::now();

    if (m_recordDebugRays) {
        clearDebugRays();
        std::cout << "\n========== SHADOW COMPUTATION WITH DEBUG RAYS ==========" << std::endl;
        std::cout << "Recording debug rays for shadow computation..." << std::endl;
    }

    std::cout << "\n=== SHADOW MAPPER: Computing shadows ===" << std::endl;
    std::cout << "Original grid cells: " << m_gridWidth << " x " << m_gridDepth << std::endl;
    std::cout << "Shadow grid cells: " << m_totalCellsX << " x " << m_totalCellsZ << std::endl;
    std::cout << "Stride: " << m_strideX << " x " << m_strideZ << std::endl;
    std::cout << "Total shadow cells: " << (m_totalCellsX * m_totalCellsZ) << std::endl;
    std::cout << "Trace mode: " << getModeName(m_shadowTraceMode) << std::endl;

    int shadowCount = 0;
    int totalCells = m_totalCellsX * m_totalCellsZ;
    int raysRecorded = 0;

    for (int sz = 0; sz < m_totalCellsZ; sz++) {
        for (int sx = 0; sx < m_totalCellsX; sx++) {
            ShadowSample& sample = m_shadowGrid[sz][sx];

            switch (m_shadowTraceMode) {
            case TRACE_CENTER:
            {
                int origCenterX = sx * m_strideX + m_strideX / 2;
                int origCenterZ = sz * m_strideZ + m_strideZ / 2;
                glm::vec3 centerPos = getCellCenter(origCenterX, origCenterZ);

                int hitCellX, hitCellZ;
                float hitDistance;
                bool inShadow = isPointInShadow(centerPos, hitCellX, hitCellZ, hitDistance);

                sample.value = inShadow ? 0.0f : 1.0f;

                if (inShadow) shadowCount++;

                if (m_recordDebugRays) {
                    glm::vec3 rayOrigin, rayDirection, endPoint;
                    if (m_lightType == LightType::Directional) {
                        rayOrigin = centerPos - m_lightDirection * 50.0f;
                        rayDirection = m_lightDirection;
                        endPoint = centerPos;
                    }
                    else {
                        rayOrigin = m_lightPos;
                        rayDirection = glm::normalize(centerPos - m_lightPos);
                        endPoint = centerPos;
                    }
                    if (inShadow && hitCellX >= 0 && hitCellZ >= 0) {
                        endPoint = getCellCenter(hitCellX, hitCellZ);
                        endPoint.y += 0.1f;
                    }
                    recordRay(rayOrigin, rayDirection, endPoint, 50.0f, inShadow, sx, sz);
                    raysRecorded++;
                }
                break;
            }

            case TRACE_CORNERS:
            {
                computeCellGradient(sample, sx, sz);
                if (sample.value < 0.5f) shadowCount++;

                if (m_recordDebugRays) {
                    raysRecorded += 4;
                }
                break;
            }

            case TRACE_CENTER_SUBDIVIDED:
            {
                computeCellCenterSubdivided(sample, sx, sz);
                if (sample.value < 0.5f) shadowCount++;
                break;
            }

            case TRACE_CORNERS_SUBDIVIDED:
            {
                computeCellCornersSubdivided(sample, sx, sz);
                if (sample.value < 0.5f) shadowCount++;
                break;
            }
            }
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

    float shadowPercent = (float)shadowCount / totalCells * 100.0f;

    std::cout << "Cells in shadow (<50%): " << shadowCount << " / " << totalCells
        << " (" << shadowPercent << "%)" << std::endl;
    std::cout << "Time: " << durationMs << " ms" << std::endl;

    if (m_recordDebugRays) {
        std::cout << "ACTUAL DEBUG RAYS RECORDED: " << raysRecorded << std::endl;
        std::cout << "m_debugRays.size(): " << m_debugRays.size() << std::endl;

        std::cout << "\n--- Ray Statistics ---" << std::endl;
        std::cout << "Center rays: " << getRaysByType(DebugRay::RAY_CENTER).size() << std::endl;
        std::cout << "Corner rays: " << getRaysByType(DebugRay::RAY_CORNER).size() << std::endl;
        std::cout << "Sub-center rays: " << getRaysByType(DebugRay::RAY_SUB_CENTER).size() << std::endl;
        std::cout << "Sub-corner rays: " << getRaysByType(DebugRay::RAY_SUB_CORNER).size() << std::endl;
        std::cout << "Hit rays (hit objects): " << getHitRays().size() << std::endl;
        std::cout << "Miss rays (reached light): " << getMissRays().size() << std::endl;
        std::cout << "---------------------" << std::endl;
    }

    std::cout << "=========================================\n" << std::endl;
}

float ShadowMapper::getShadowAtCell(int cellX, int cellZ) const
{
    int shadowX = cellX / m_strideX;
    int shadowZ = cellZ / m_strideZ;

    if (shadowX < 0 || shadowX >= m_totalCellsX || shadowZ < 0 || shadowZ >= m_totalCellsZ) {
        return 1.0f;
    }
    return m_shadowGrid[shadowZ][shadowX].value;
}

float ShadowMapper::getShadowAtCellGradient(int cellX, int cellZ, float& outR, float& outG, float& outB) const
{
    int shadowX = cellX / m_strideX;
    int shadowZ = cellZ / m_strideZ;

    if (shadowX < 0 || shadowX >= m_totalCellsX || shadowZ < 0 || shadowZ >= m_totalCellsZ) {
        outR = outG = outB = 1.0f;
        return 1.0f;
    }

    const ShadowSample& sample = m_shadowGrid[shadowZ][shadowX];

    if ((m_shadowTraceMode == TRACE_CORNERS || m_shadowTraceMode == TRACE_CORNERS_SUBDIVIDED) && sample.useGradient) {
        outR = sample.value;
        outG = sample.value;
        outB = sample.value;
        return sample.value;
    }

    outR = outG = outB = sample.value;
    return sample.value;
}

glm::vec3 ShadowMapper::getShadowColorAtCell(int cellX, int cellZ) const
{
    int shadowX = cellX / m_strideX;
    int shadowZ = cellZ / m_strideZ;

    if (shadowX < 0 || shadowX >= m_totalCellsX || shadowZ < 0 || shadowZ >= m_totalCellsZ) {
        return glm::vec3(1.0f);
    }

    const ShadowSample& sample = m_shadowGrid[shadowZ][shadowX];

    if ((m_shadowTraceMode == TRACE_CORNERS || m_shadowTraceMode == TRACE_CORNERS_SUBDIVIDED) && sample.useGradient) {
        glm::vec3 darkColor(0.05f, 0.05f, 0.03f);
        glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
        return glm::mix(darkColor, lightColor, sample.value);
    }

    float val = sample.value;
    return glm::vec3(val);
}

float ShadowMapper::getShadowAtPoint(const glm::vec3& point) const
{
    float halfWidth = m_gridWidth * m_cellSize / 2.0f;
    float halfDepth = m_gridDepth * m_cellSize / 2.0f;

    int cellX = (int)((point.x + halfWidth) / m_cellSize);
    int cellZ = (int)((point.z + halfDepth) / m_cellSize);

    cellX = std::max(0, std::min(cellX, m_gridWidth - 1));
    cellZ = std::max(0, std::min(cellZ, m_gridDepth - 1));

    int shadowX = cellX / m_strideX;
    int shadowZ = cellZ / m_strideZ;

    if (shadowX < 0 || shadowX >= m_totalCellsX || shadowZ < 0 || shadowZ >= m_totalCellsZ) {
        return 1.0f;
    }
    return m_shadowGrid[shadowZ][shadowX].value;
}

glm::vec3 ShadowMapper::getShadowColorAtPoint(const glm::vec3& point) const
{
    float halfWidth = m_gridWidth * m_cellSize / 2.0f;
    float halfDepth = m_gridDepth * m_cellSize / 2.0f;

    int cellX = (int)((point.x + halfWidth) / m_cellSize);
    int cellZ = (int)((point.z + halfDepth) / m_cellSize);

    cellX = std::max(0, std::min(cellX, m_gridWidth - 1));
    cellZ = std::max(0, std::min(cellZ, m_gridDepth - 1));

    return getShadowColorAtCell(cellX, cellZ);
}

float ShadowMapper::getShadowAtWorldPos(float x, float z) const
{
    return getShadowAtPoint(glm::vec3(x, 0.0f, z));
}

bool ShadowMapper::isInGridBounds(int x, int z) const
{
    return x >= 0 && x < m_totalCellsX && z >= 0 && z < m_totalCellsZ;
}