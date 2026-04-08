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
    , m_strideX(1)
    , m_strideZ(20)
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
    int strideX, int strideZ)
    : m_gridWidth(width)
    , m_gridDepth(depth)
    , m_cellSize(cellSize)
    , m_groundHeight(groundHeight)
    , m_strideX(3)
    , m_strideZ(3)
    , m_totalSamplesX(width / m_strideX)
    , m_totalSamplesZ(depth / m_strideZ)
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

    m_totalSamplesX = m_gridWidth / m_strideX;
    m_totalSamplesZ = m_gridDepth / m_strideZ;

    std::cout << "=== ShadowMapper Grid Setup ===" << std::endl;
    std::cout << "Game grid cells: " << m_gridWidth << " x " << m_gridDepth << std::endl;
    std::cout << "Stride (ratio): " << m_strideX << ":" << m_strideZ << std::endl;
    std::cout << "Shadow samples: " << m_totalSamplesX << " x " << m_totalSamplesZ << std::endl;
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

    // Преобразуем индекс сэмпла в индекс клетки игровой сетки
    // С учётом stride (шага)
    int cellX = sampleX * m_strideX + (m_strideX / 2);
    int cellZ = sampleZ * m_strideZ + (m_strideZ / 2);

    // Ограничиваем, чтобы не выйти за пределы
    cellX = std::min(cellX, m_gridWidth - 1);
    cellZ = std::min(cellZ, m_gridDepth - 1);

    float worldX = cellX * m_cellSize - halfWidth;
    float worldZ = cellZ * m_cellSize - halfDepth;

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

bool ShadowMapper::isVertexInShadow(const glm::vec3& position)
{
    glm::vec3 rayOrigin = position + glm::vec3(0.0f, 0.02f, 0.0f);
    Ray shadowRay(rayOrigin, -m_lightDirection);

    float sphereHitDistance;
    if (intersectsAnyBoundingSphere(shadowRay, sphereHitDistance)) {
        if (m_intersectCallback) {
            float exactHitDistance;
            glm::vec3 hitPoint;
            if (m_intersectCallback(shadowRay, exactHitDistance, hitPoint)) {
                return true;
            }
        }
        return true;
    }

    return false;
}

void ShadowMapper::computeShadows()
{
    if (m_totalSamplesX == 0 || m_totalSamplesZ == 0) return;

    auto startTime = std::chrono::high_resolution_clock::now();

    std::cout << "\n=== SHADOW MAPPER: Computing shadows ===" << std::endl;
    std::cout << "Game grid: " << m_gridWidth << " x " << m_gridDepth << std::endl;
    std::cout << "Stride: " << m_strideX << ":" << m_strideZ << std::endl;
    std::cout << "Shadow samples: " << m_totalSamplesX << " x " << m_totalSamplesZ << std::endl;
    std::cout << "Total samples: " << (m_totalSamplesX * m_totalSamplesZ) << std::endl;

    int totalVertices = m_totalSamplesX * m_totalSamplesZ;
    int shadowCount = 0;

    for (int sz = 0; sz < m_totalSamplesZ; sz++) {
        for (int sx = 0; sx < m_totalSamplesX; sx++) {
            ShadowSample& sample = m_shadowGrid[sz][sx];

            if (isVertexInShadow(sample.position)) {
                sample.value = 0.3f;
                shadowCount++;
            }
            else {
                sample.value = 1.0f;
            }

            sample.computed = true;
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

    float shadowPercent = (float)shadowCount / totalVertices * 100.0f;

    std::cout << "Shadows: " << shadowCount << " / " << totalVertices
        << " (" << shadowPercent << "% in shadow)" << std::endl;
    std::cout << "Time: " << durationMs << " ms" << std::endl;
    std::cout << "=========================================\n" << std::endl;
}

float ShadowMapper::bilinearInterpolate(float x, float z) const
{
    float halfWidth = m_gridWidth * m_cellSize / 2.0f;
    float halfDepth = m_gridDepth * m_cellSize / 2.0f;

    // Преобразуем мировые координаты в индексы сэмплов
    float sampleX = (x + halfWidth) / (m_cellSize * m_strideX);
    float sampleZ = (z + halfDepth) / (m_cellSize * m_strideZ);

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