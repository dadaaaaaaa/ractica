#include "../pch.h"
#include "ShadowMapper.h"
#include <algorithm>
#include <cmath>
#include <chrono>
#include "RayTracer.h"

ShadowMapper::ShadowMapper()
    : m_gridWidth(0)
    , m_gridDepth(0)
    , m_cellSize(0.1f)
    , m_groundHeight(0.0f)
    , m_samplesPerCellX(1)
    , m_samplesPerCellZ(1)
    , m_totalSamplesX(0)
    , m_totalSamplesZ(0)
    , m_lightDirection(1.0f, 0.2f, 0.5f)
    , m_lightColor(0.9f, 0.85f, 0.75f)
{
    m_lightDirection = glm::normalize(m_lightDirection);
    setGrid(0, 0, 0.1f, 0.0f);
}

ShadowMapper::ShadowMapper(int width, int depth, float cellSize, float groundHeight,
    glm::vec3 lightDirection, glm::vec3 lightColor,
    int samplesPerCellX, int samplesPerCellZ)
    : m_gridWidth(width)
    , m_gridDepth(depth)
    , m_cellSize(cellSize)
    , m_groundHeight(groundHeight)
    , m_samplesPerCellX(std::max(1, samplesPerCellX))
    , m_samplesPerCellZ(std::max(1, samplesPerCellZ))
    , m_totalSamplesX(width* m_samplesPerCellX)
    , m_totalSamplesZ(depth* m_samplesPerCellZ)
    , m_lightDirection(glm::normalize(lightDirection))
    , m_lightColor(lightColor)
{
    setGrid(width, depth, cellSize, groundHeight);
}

void ShadowMapper::setGrid(int width, int depth, float cellSize, float groundHeight)
{
    m_gridWidth = width;
    m_gridDepth = depth;
    m_cellSize = cellSize;
    m_groundHeight = groundHeight;

    m_totalSamplesX = m_gridWidth * m_samplesPerCellX;
    m_totalSamplesZ = m_gridDepth * m_samplesPerCellZ;

    std::cout << "=== ShadowMapper Grid Setup ===" << std::endl;
    std::cout << "Grid cells: " << m_gridWidth << " x " << m_gridDepth << std::endl;
    std::cout << "Samples per cell: " << m_samplesPerCellX << " x " << m_samplesPerCellZ << std::endl;
    std::cout << "Total samples: " << m_totalSamplesX << " x " << m_totalSamplesZ << std::endl;
    std::cout << "===============================" << std::endl;

    m_shadowGrid.resize(m_totalSamplesZ, std::vector<ShadowSample>(m_totalSamplesX));

    for (int sz = 0; sz < m_totalSamplesZ; sz++) {
        for (int sx = 0; sx < m_totalSamplesX; sx++) {
            m_shadowGrid[sz][sx].position = getVertexPosition(sx, sz);
            m_shadowGrid[sz][sx].computed = false;
            m_shadowGrid[sz][sx].value = 1.0f;
        }
    }
}

glm::vec3 ShadowMapper::getVertexPosition(int sampleX, int sampleZ) const
{
    float halfWidth = m_gridWidth * m_cellSize / 2.0f;
    float halfDepth = m_gridDepth * m_cellSize / 2.0f;

    int cellX = sampleX / m_samplesPerCellX;
    int cellZ = sampleZ / m_samplesPerCellZ;

    float offsetInCellX = (float)(sampleX % m_samplesPerCellX) / m_samplesPerCellX;
    float offsetInCellZ = (float)(sampleZ % m_samplesPerCellZ) / m_samplesPerCellZ;

    float cellCenterX = cellX * m_cellSize - halfWidth + m_cellSize / 2.0f;
    float cellCenterZ = cellZ * m_cellSize - halfDepth + m_cellSize / 2.0f;

    float offsetX = (offsetInCellX - 0.5f) * m_cellSize;
    float offsetZ = (offsetInCellZ - 0.5f) * m_cellSize;

    float worldX = cellCenterX + offsetX;
    float worldZ = cellCenterZ + offsetZ;

    return glm::vec3(worldX, m_groundHeight, worldZ);
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

void ShadowMapper::setIntersectCallback(std::function<bool(const Ray&, float&, glm::vec3&)> callback)
{
    m_intersectCallback = callback;
}

bool ShadowMapper::intersectsAnyBoundingSphere(const Ray& ray, float& hitDistance)
{
    float closestHit = 1000.0f;
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

            if (t1 > 0.001f && t1 < closestHit) {
                closestHit = t1;
                hit = true;
            }
            if (t2 > 0.001f && t2 < closestHit) {
                closestHit = t2;
                hit = true;
            }
        }
    }

    if (hit) {
        hitDistance = closestHit;
    }
    return hit;
}

// ПРОСТАЯ ПРОВЕРКА ТЕНИ - БЕЗ СЛУЧАЙНЫХ СМЕЩЕНИЙ
bool ShadowMapper::isVertexInShadow(const glm::vec3& position)
{
    // Луч от точки к источнику света (без случайных смещений)
    glm::vec3 rayOrigin = position + glm::vec3(0.0f, 0.02f, 0.0f);
    Ray shadowRay(rayOrigin, -m_lightDirection);

    float sphereHitDistance;
    if (intersectsAnyBoundingSphere(shadowRay, sphereHitDistance)) {
        if (m_intersectCallback) {
            float exactHitDistance;
            glm::vec3 hitPoint;
            if (m_intersectCallback(shadowRay, exactHitDistance, hitPoint)) {
                return true;  // В тени
            }
        }
        return true;  // В тени
    }

    return false;  // На свету
}

void ShadowMapper::computeShadows()
{
    if (m_totalSamplesX == 0 || m_totalSamplesZ == 0) return;

    auto startTime = std::chrono::high_resolution_clock::now();

    std::cout << "\n=== SHADOW MAPPER: Computing shadows ===" << std::endl;
    std::cout << "Grid cells: " << m_gridWidth << " x " << m_gridDepth << std::endl;
    std::cout << "Samples per cell: " << m_samplesPerCellX << " x " << m_samplesPerCellZ << std::endl;
    std::cout << "Total samples: " << m_totalSamplesX << " x " << m_totalSamplesZ << std::endl;
    std::cout << "Total vertices to process: " << (m_totalSamplesX * m_totalSamplesZ) << std::endl;
    std::cout << "Cell size: " << m_cellSize << std::endl;
    std::cout << "Light direction: (" << m_lightDirection.x << ", "
        << m_lightDirection.y << ", " << m_lightDirection.z << ")" << std::endl;

    int totalVertices = m_totalSamplesX * m_totalSamplesZ;
    int shadowCount = 0;

    bool showProgress = (totalVertices > 10000);
    int lastPercent = 0;

    // Простой проход - один луч на точку, без случайных смещений
    for (int sz = 0; sz < m_totalSamplesZ; sz++) {
        for (int sx = 0; sx < m_totalSamplesX; sx++) {
            ShadowSample& sample = m_shadowGrid[sz][sx];

            if (isVertexInShadow(sample.position)) {
                sample.value = 0.3f;  // В тени - 30% света
                shadowCount++;
            }
            else {
                sample.value = 1.0f;  // На свету - 100% света
            }

            sample.computed = true;
        }

        if (showProgress) {
            int percent = (sz * 100) / m_totalSamplesZ;
            if (percent != lastPercent && percent % 10 == 0) {
                std::cout << "  Progress: " << percent << "% (" << sz << "/" << m_totalSamplesZ << " rows)" << std::endl;
                lastPercent = percent;
            }
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

    float shadowPercent = (float)shadowCount / totalVertices * 100.0f;

    std::cout << "Shadows computed: " << shadowCount << " / " << totalVertices
        << " (" << shadowPercent << "% in shadow)" << std::endl;
    std::cout << "Time taken: " << durationMs << " ms" << std::endl;
    std::cout << "=========================================\n" << std::endl;
}

float ShadowMapper::getShadowAtSample(int sampleX, int sampleZ) const
{
    if (sampleX < 0 || sampleX >= m_totalSamplesX ||
        sampleZ < 0 || sampleZ >= m_totalSamplesZ) {
        return 1.0f;
    }
    return m_shadowGrid[sampleZ][sampleX].value;
}

float ShadowMapper::bilinearInterpolate(float x, float z) const
{
    float halfWidth = m_gridWidth * m_cellSize / 2.0f;
    float halfDepth = m_gridDepth * m_cellSize / 2.0f;

    float sampleX = (x + halfWidth) / m_cellSize * m_samplesPerCellX;
    float sampleZ = (z + halfDepth) / m_cellSize * m_samplesPerCellZ;

    int x0 = (int)floor(sampleX);
    int z0 = (int)floor(sampleZ);
    int x1 = x0 + 1;
    int z1 = z0 + 1;

    if (x0 < 0 || x1 >= m_totalSamplesX || z0 < 0 || z1 >= m_totalSamplesZ) {
        int cx = std::max(0, std::min(m_totalSamplesX - 1, x0));
        int cz = std::max(0, std::min(m_totalSamplesZ - 1, z0));
        return m_shadowGrid[cz][cx].value;
    }

    float fx = sampleX - x0;
    float fz = sampleZ - z0;

    float v00 = m_shadowGrid[z0][x0].value;
    float v10 = m_shadowGrid[z0][x1].value;
    float v01 = m_shadowGrid[z1][x0].value;
    float v11 = m_shadowGrid[z1][x1].value;

    float top = v00 * (1.0f - fx) + v10 * fx;
    float bottom = v01 * (1.0f - fx) + v11 * fx;

    return top * (1.0f - fz) + bottom * fz;
}

float ShadowMapper::getShadowAtPoint(const glm::vec3& point) const
{
    return bilinearInterpolate(point.x, point.z);
}

float ShadowMapper::getShadowAtWorldPos(float x, float z) const
{
    return bilinearInterpolate(x, z);
}

bool ShadowMapper::isInGridBounds(int x, int z) const
{
    return x >= 0 && x < m_totalSamplesX && z >= 0 && z < m_totalSamplesZ;
}

int ShadowMapper::worldToGridX(float worldX) const
{
    float halfWidth = m_gridWidth * m_cellSize / 2.0f;
    return (int)((worldX + halfWidth) / m_cellSize);
}

int ShadowMapper::worldToGridZ(float worldZ) const
{
    float halfDepth = m_gridDepth * m_cellSize / 2.0f;
    return (int)((worldZ + halfDepth) / m_cellSize);
}

int ShadowMapper::worldToSampleX(float worldX) const
{
    float halfWidth = m_gridWidth * m_cellSize / 2.0f;
    float gridX = (worldX + halfWidth) / m_cellSize;
    return (int)(gridX * m_samplesPerCellX);
}

int ShadowMapper::worldToSampleZ(float worldZ) const
{
    float halfDepth = m_gridDepth * m_cellSize / 2.0f;
    float gridZ = (worldZ + halfDepth) / m_cellSize;
    return (int)(gridZ * m_samplesPerCellZ);
}