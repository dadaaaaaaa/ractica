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
    std::cout << "Trace mode: " << (m_shadowTraceMode == TRACE_CENTER ? "CENTER" : "CORNERS") << std::endl;
    std::cout << "===============================" << std::endl;

    // Инициализируем сетку теней (уменьшенного размера)
    m_shadowGrid.resize(m_totalCellsZ, std::vector<ShadowSample>(m_totalCellsX));

    for (int z = 0; z < m_totalCellsZ; z++) {
        for (int x = 0; x < m_totalCellsX; x++) {
            m_shadowGrid[z][x].cellX = x;
            m_shadowGrid[z][x].cellZ = z;
            m_shadowGrid[z][x].useGradient = (m_shadowTraceMode == TRACE_CORNERS);
            // Позиция центра клетки в оригинальной сетке
            int origX = x * m_strideX + m_strideX / 2;
            int origZ = z * m_strideZ + m_strideZ / 2;
            m_shadowGrid[z][x].position = getCellCenter(origX, origZ);
            m_shadowGrid[z][x].computed = false;
            m_shadowGrid[z][x].value = 1.0f;
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

    // cellX и cellZ - индексы оригинальной сетки
    float worldX = cellX * m_cellSize - halfWidth + (m_cellSize / 2.0f);
    float worldZ = cellZ * m_cellSize - halfDepth + (m_cellSize / 2.0f);

    return glm::vec3(worldX, m_groundHeight + 0.05f, worldZ);
}

glm::vec3 ShadowMapper::getCornerWorldPosition(int cellX, int cellZ, int cornerIndex) const
{
    float halfWidth = m_gridWidth * m_cellSize / 2.0f;
    float halfDepth = m_gridDepth * m_cellSize / 2.0f;

    // ВНИМАНИЕ: cellX и cellZ - это индексы В СЕТКЕ ТЕНЕЙ (уменьшенной)
    // Нужно пересчитать в индексы оригинальной сетки
    int origCellX = cellX * m_strideX;
    int origCellZ = cellZ * m_strideZ;

    // Координаты углов клетки в локальных координатах сетки
    float worldX = origCellX * m_cellSize - halfWidth;
    float worldZ = origCellZ * m_cellSize - halfDepth;

    // Ширина клетки в оригинальной сетке с учётом stride
    float cellWidthX = m_cellSize * m_strideX;
    float cellWidthZ = m_cellSize * m_strideZ;

    switch (cornerIndex) {
    case 0: // Bottom-Left (нижний-левый) - X мин, Z мин
        return glm::vec3(worldX, m_groundHeight + 0.05f, worldZ);
    case 1: // Bottom-Right (нижний-правый) - X макс, Z мин
        return glm::vec3(worldX + cellWidthX, m_groundHeight + 0.05f, worldZ);
    case 2: // Top-Left (верхний-левый) - X мин, Z макс
        return glm::vec3(worldX, m_groundHeight + 0.05f, worldZ + cellWidthZ);
    case 3: // Top-Right (верхний-правый) - X макс, Z макс
        return glm::vec3(worldX + cellWidthX, m_groundHeight + 0.05f, worldZ + cellWidthZ);
    default:
        return glm::vec3(worldX, m_groundHeight + 0.05f, worldZ);
    }
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

    // Проверка сфер объектов
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

    // Если есть callback для более точной геометрии
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
        rayOrigin = point - m_lightDirection * 100.0f;
        rayDirection = m_lightDirection;
        maxRayDistance = 100.0f;
        break;

    case LightType::Points:
    {
        glm::vec3 toLight = m_lightPos - point;
        float distanceToLight = glm::length(toLight);

        if (distanceToLight < 0.01f) {
            hitCellX = -1;
            hitCellZ = -1;
            hitDistance = 0;
            return false;
        }

        rayOrigin = point;
        rayDirection = glm::normalize(toLight);
        maxRayDistance = distanceToLight;

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

        // Если точка вне конуса прожектора - она в тени
        if (cosAngle < spotCos) {
            hitDistance = distanceToPoint;
            hitCellX = -1;
            hitCellZ = -1;
            return true;
        }

        rayOrigin = point;
        rayDirection = -toPointDir;
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
    // Собираем значения теней для 4 углов (0 = в тени, 1 = на свету)
    float shadowSum = 0.0f;
    int hitCount = 0;  // Количество углов НА СВЕТУ (1)
    int shadowCount = 0; // Количество углов В ТЕНИ (0)

    for (int corner = 0; corner < 4; corner++) {
        glm::vec3 cornerPos = getCornerWorldPosition(cellX, cellZ, corner);
        int hitCellX, hitCellZ;
        float hitDistance;

        bool inShadow = isPointInShadow(cornerPos, hitCellX, hitCellZ, hitDistance);

        // cornerShadows: 0 = тень, 1 = свет
        sample.cornerShadows[corner] = inShadow ? 0.0f : 1.0f;
        shadowSum += sample.cornerShadows[corner];

        if (inShadow) {
            shadowCount++;
        }
        else {
            hitCount++;
        }

        // Записываем отладочный луч для каждого угла
        if (m_recordDebugRays) {
            glm::vec3 rayOrigin, rayDirection, endPoint;

            if (m_lightType == LightType::Directional) {
                rayOrigin = cornerPos;
                rayDirection = -m_lightDirection;
                endPoint = rayOrigin + rayDirection * 50.0f;
            }
            else {
                rayOrigin = cornerPos;
                rayDirection = glm::normalize(m_lightPos - cornerPos);
                endPoint = m_lightPos;
            }

            if (inShadow && hitCellX >= 0 && hitCellZ >= 0) {
                endPoint = getCellCenter(hitCellX, hitCellZ);
                endPoint.y += 0.1f;
            }

            recordRay(rayOrigin, rayDirection, endPoint, 50.0f, inShadow, cellX, cellZ, corner);
        }
    }

    // Логика градиента:
    // - Если все 4 угла на свету (hitCount == 4) -> полностью светлая клетка (value = 1.0)
    // - Если все 4 угла в тени (shadowCount == 4) -> полностью тёмная клетка (value = 0.0)
    // - Иначе градиент: value = количество освещённых углов / 4
    sample.value = (float)hitCount / 4.0f;

    // Дополнительная информация для отладки
    if (m_recordDebugRays && hitCount > 0 && hitCount < 4) {
        std::cout << "Cell [" << cellX << "," << cellZ << "] - Gradient: "
            << hitCount << "/4 corners lit, value = " << sample.value << std::endl;
    }
}

void ShadowMapper::recordRay(const glm::vec3& origin, const glm::vec3& direction,
    const glm::vec3& hitPoint, float distance, bool hit, int cellX, int cellZ, int cornerIndex)
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
    std::cout << "Trace mode: " << (m_shadowTraceMode == TRACE_CENTER ? "CENTER (1 ray per cell)" : "CORNERS (4 rays per cell)") << std::endl;

    if (m_recordDebugRays) {
        if (m_shadowTraceMode == TRACE_CORNERS) {
            std::cout << "EXPECTED DEBUG RAYS: " << (m_totalCellsX * m_totalCellsZ * 4) << " rays" << std::endl;
        }
        else {
            std::cout << "EXPECTED DEBUG RAYS: " << (m_totalCellsX * m_totalCellsZ) << " rays" << std::endl;
        }
    }

    int shadowCount = 0;
    int totalCells = m_totalCellsX * m_totalCellsZ;
    int raysRecorded = 0;

    for (int sz = 0; sz < m_totalCellsZ; sz++) {
        for (int sx = 0; sx < m_totalCellsX; sx++) {
            ShadowSample& sample = m_shadowGrid[sz][sx];

            if (m_shadowTraceMode == TRACE_CENTER) {
                // Режим 1: Один луч в центр клетки
                int origCenterX = sx * m_strideX + m_strideX / 2;
                int origCenterZ = sz * m_strideZ + m_strideZ / 2;
                glm::vec3 centerPos = getCellCenter(origCenterX, origCenterZ);

                int hitCellX, hitCellZ;
                float hitDistance;
                bool inShadow = isPointInShadow(centerPos, hitCellX, hitCellZ, hitDistance);

                if (inShadow) {
                    sample.value = 0.0f;
                    shadowCount++;
                }
                else {
                    sample.value = 1.0f;
                }

                if (m_recordDebugRays) {
                    glm::vec3 rayOrigin, rayDirection, endPoint;

                    if (m_lightType == LightType::Directional) {
                        rayOrigin = centerPos;
                        rayDirection = -m_lightDirection;
                        endPoint = rayOrigin + rayDirection * 50.0f;
                    }
                    else {
                        rayOrigin = centerPos;
                        rayDirection = glm::normalize(m_lightPos - centerPos);
                        endPoint = m_lightPos;
                    }

                    if (inShadow && hitCellX >= 0 && hitCellZ >= 0) {
                        endPoint = getCellCenter(hitCellX, hitCellZ);
                        endPoint.y += 0.1f;
                    }

                    recordRay(rayOrigin, rayDirection, endPoint, 50.0f, inShadow, sx, sz);
                    raysRecorded++;
                }
            }
            else // TRACE_CORNERS
            {
                // Режим 2: Четыре луча по углам клетки
                float shadowSum = 0.0f;
                int litCorners = 0;  // Считаем освещённые углы

                for (int corner = 0; corner < 4; corner++) {
                    glm::vec3 cornerPos = getCornerWorldPosition(sx, sz, corner);
                    int hitCellX, hitCellZ;
                    float hitDistance;

                    bool inShadow = isPointInShadow(cornerPos, hitCellX, hitCellZ, hitDistance);

                    sample.cornerShadows[corner] = inShadow ? 0.0f : 1.0f;
                    shadowSum += sample.cornerShadows[corner];

                    if (!inShadow) {
                        litCorners++;
                    }

                    if (m_recordDebugRays) {
                        glm::vec3 rayOrigin, rayDirection, endPoint;

                        if (m_lightType == LightType::Directional) {
                            rayOrigin = cornerPos;
                            rayDirection = -m_lightDirection;
                            endPoint = rayOrigin + rayDirection * 50.0f;
                        }
                        else {
                            rayOrigin = cornerPos;
                            rayDirection = glm::normalize(m_lightPos - cornerPos);
                            endPoint = m_lightPos;
                        }

                        if (inShadow && hitCellX >= 0 && hitCellZ >= 0) {
                            endPoint = getCellCenter(hitCellX, hitCellZ);
                            endPoint.y += 0.1f;
                        }

                        recordRay(rayOrigin, rayDirection, endPoint, 50.0f, inShadow, sx, sz, corner);
                        raysRecorded++;
                    }
                }

                // Значение тени = количество освещённых углов / 4
                // 0.0 = все углы в тени, 0.25 = 1 угол освещён, 0.5 = 2 угла, 0.75 = 3 угла, 1.0 = все освещены
                sample.value = (float)litCorners / 4.0f;

                if (litCorners < 2) {  // 0 или 1 угол освещён - считаем клетку в тени
                    shadowCount++;
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
    }

    std::cout << "=========================================\n" << std::endl;
}

float ShadowMapper::getShadowAtCell(int cellX, int cellZ) const
{
    // cellX и cellZ - индексы оригинальной сетки
    // Пересчитываем в индексы сетки теней
    int shadowX = cellX / m_strideX;
    int shadowZ = cellZ / m_strideZ;

    if (shadowX < 0 || shadowX >= m_totalCellsX || shadowZ < 0 || shadowZ >= m_totalCellsZ) {
        return 1.0f;
    }
    return m_shadowGrid[shadowZ][shadowX].value;
}

float ShadowMapper::getShadowAtCellGradient(int cellX, int cellZ, float& outR, float& outG, float& outB) const
{
    // cellX и cellZ - индексы оригинальной сетки
    // Пересчитываем в индексы сетки теней
    int shadowX = cellX / m_strideX;
    int shadowZ = cellZ / m_strideZ;

    if (shadowX < 0 || shadowX >= m_totalCellsX || shadowZ < 0 || shadowZ >= m_totalCellsZ) {
        outR = outG = outB = 1.0f;
        return 1.0f;
    }

    const ShadowSample& sample = m_shadowGrid[shadowZ][shadowX];

    if (m_shadowTraceMode == TRACE_CORNERS && sample.useGradient) {
        // НЕ ИСПОЛЬЗУЕМ floorColor ЗДЕСЬ!
        // Вместо этого возвращаем ТОЛЬКО коэффициент освещения (sample.value)
        // А цвет пола будет применён в GameRenderer

        outR = sample.value;
        outG = sample.value;
        outB = sample.value;
        return sample.value;
    }

    // Для обычного режима
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

    if (m_shadowTraceMode == TRACE_CORNERS && sample.useGradient) {
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