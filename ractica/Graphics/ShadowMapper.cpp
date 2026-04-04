#include "../pch.h"
#include "ShadowMapper.h"
#include <algorithm>
#include <cmath>
#include "RayTracer.h"
ShadowMapper::ShadowMapper()
    : m_gridWidth(0)
    , m_gridDepth(0)
    , m_cellSize(0.1f)
    , m_groundHeight(0.0f)
    , m_lightDirection(1.0f, 0.2f, 0.5f)
    , m_lightColor(0.9f, 0.85f, 0.75f)
{
    m_lightDirection = glm::normalize(m_lightDirection);
    setGrid(0, 0, 0.1f, 0.0f);
}
ShadowMapper::ShadowMapper(int width,
    int depth,
    float cellSize,
    float groundHeight,
    glm::vec3 lightDirection,
    glm::vec3 lightColor)
    : m_gridWidth(width)
    , m_gridDepth(depth)
    , m_cellSize(cellSize)
    , m_groundHeight(groundHeight)
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

    // Инициализируем сетку теней
    m_shadowGrid.resize(m_gridDepth, std::vector<ShadowSample>(m_gridWidth));

    // Заполняем позиции вершин
    for (int z = 0; z < m_gridDepth; z++) {
        for (int x = 0; x < m_gridWidth; x++) {
            m_shadowGrid[z][x].position = getVertexPosition(x, z);
            m_shadowGrid[z][x].computed = false;
            m_shadowGrid[z][x].value = 1.0f; // По умолчанию полный свет
        }
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

void ShadowMapper::setIntersectCallback(std::function<bool(const Ray&, float&, glm::vec3&)> callback)
{
    m_intersectCallback = callback;
}

bool ShadowMapper::intersectsAnyBoundingSphere(const Ray& ray, float& hitDistance)
{
    float closestHit = 1000.0f;
    bool hit = false;

    for (const auto& sphere : m_objectSpheres) {
        // Проверка пересечения луча со сферой
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
    // Сдвигаем начало луча чуть выше поверхности, чтобы избежать self-intersection
    glm::vec3 rayOrigin = position - m_lightDirection * 0.01f;
    Ray shadowRay(rayOrigin, -m_lightDirection);

    // БЫСТРЫЙ ТЕСТ 1: Bounding spheres
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
float ShadowMapper::computeShadowFactor(const glm::vec3& position)
{
    int samples = 8; // количество лучей
    int hits = 0;

    float spread = 0.05f; // размытие тени

    for (int i = 0; i < samples; i++) {

        // небольшой разброс луча
        static const glm::vec3 offsets[8] = {
            {0.02f,0,0.02f}, {-0.02f,0,0.02f},
            {0.02f,0,-0.02f}, {-0.02f,0,-0.02f},
            {0.04f,0,0}, {-0.04f,0,0},
            {0,0,0.04f}, {0,0,-0.04f}
        };

        glm::vec3 rayOrigin = position + offsets[i] - m_lightDirection * 0.01f;

        Ray shadowRay;

        if (m_lightType == LightType::Directional) {
            shadowRay = Ray(rayOrigin, -m_lightDirection);
        }
        else {
            glm::vec3 dir = glm::normalize(m_lightPos - rayOrigin);
            shadowRay = Ray(rayOrigin, dir);
        }

        float dist;
        if (intersectsAnyBoundingSphere(shadowRay, dist)) {
            hits++;
        }
    }

    // 1.0 = свет, 0.0 = полная тень
    float shadow = 1.0f - (float)hits / samples;

    return shadow;
}
void ShadowMapper::computeShadows()
{
    if (m_gridWidth == 0 || m_gridDepth == 0) return;

    std::cout << "\n=== SHADOW MAPPER: Computing shadows ===" << std::endl;
    std::cout << "Grid size: " << m_gridWidth << " x " << m_gridDepth << std::endl;
    std::cout << "Cell size: " << m_cellSize << std::endl;
    std::cout << "Light direction: (" << m_lightDirection.x << ", "
        << m_lightDirection.y << ", " << m_lightDirection.z << ")" << std::endl;

    int totalVertices = m_gridWidth * m_gridDepth;
    int shadowCount = 0;

    // Проходим по всем вершинам сетки
    for (int z = 0; z < m_gridDepth; z++) {
        for (int x = 0; x < m_gridWidth; x++) {
            ShadowSample& sample = m_shadowGrid[z][x];
            sample.value = computeShadowFactor(sample.position);
            //для квадратных теней
            //if (isVertexInShadow(sample.position)) {
            //    sample.value = 0.6f; // Тень - только 30% света
            //    shadowCount++;
            //}
            //else {
            //    sample.value = 1.0f; // Полный свет
            //}

            sample.computed = true;
        }
    }

    float shadowPercent = (float)shadowCount / totalVertices * 100.0f;
    std::cout << "Shadows computed: " << shadowCount << " / " << totalVertices
        << " (" << shadowPercent << "% in shadow)" << std::endl;
    std::cout << "=========================================\n" << std::endl;
}

float ShadowMapper::bilinearInterpolate(float x, float z) const
{
    // Конвертируем мировые координаты в индексы сетки
    float halfWidth = m_gridWidth * m_cellSize / 2.0f;
    float halfDepth = m_gridDepth * m_cellSize / 2.0f;

    float gridX = (x + halfWidth) / m_cellSize;
    float gridZ = (z + halfDepth) / m_cellSize;

    int x0 = (int)floor(gridX);
    int z0 = (int)floor(gridZ);
    int x1 = x0 + 1;
    int z1 = z0 + 1;

    // Проверка границ
    if (x0 < 0 || x1 >= m_gridWidth || z0 < 0 || z1 >= m_gridDepth) {
        // Возвращаем ближайшую существующую вершину
        int cx = std::max(0, std::min(m_gridWidth - 1, x0));
        int cz = std::max(0, std::min(m_gridDepth - 1, z0));
        return m_shadowGrid[cz][cx].value;
    }

    // Коэффициенты интерполяции
    float fx = gridX - x0;
    float fz = gridZ - z0;

    // Значения в 4 углах
    float v00 = m_shadowGrid[z0][x0].value;
    float v10 = m_shadowGrid[z0][x1].value;
    float v01 = m_shadowGrid[z1][x0].value;
    float v11 = m_shadowGrid[z1][x1].value;

    // Билинейная интерполяция
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

glm::vec3 ShadowMapper::getVertexPosition(int x, int z) const
{
    float halfWidth = m_gridWidth * m_cellSize / 2.0f;
    float halfDepth = m_gridDepth * m_cellSize / 2.0f;

    float worldX = x * m_cellSize - halfWidth;
    float worldZ = z * m_cellSize - halfDepth;

    return glm::vec3(worldX, m_groundHeight, worldZ);
}

bool ShadowMapper::isInGridBounds(int x, int z) const
{
    return x >= 0 && x < m_gridWidth && z >= 0 && z < m_gridDepth;
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