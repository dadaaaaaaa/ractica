#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <functional>

struct Ray;

struct ShadowSample {
    bool computed;
    float value;        // 0.0 = в тени, 1.0 = на свету
    glm::vec3 position;

    ShadowSample() : computed(false), value(1.0f), position(0.0f) {}
};

struct BoundingSphere {
    glm::vec3 center;
    float radius;

    BoundingSphere() : center(0.0f), radius(0.0f) {}
    BoundingSphere(const glm::vec3& c, float r) : center(c), radius(r) {}
};

class ShadowMapper {
private:
    int m_gridWidth;        // Количество клеток в игровой сетке
    int m_gridDepth;
    float m_cellSize;
    float m_groundHeight;

    // Соотношение: 1:1, 1:2, 2:1
    int m_strideX;          // Шаг по X (1 = каждый, 2 = каждый второй)
    int m_strideZ;          // Шаг по Z

    int m_totalSamplesX;    // Всего сэмплов = gridWidth / strideX
    int m_totalSamplesZ;    // Всего сэмплов = gridDepth / strideZ

    std::vector<std::vector<ShadowSample>> m_shadowGrid;
    LightType m_lightType;
    glm::vec3 m_lightPos;
    glm::vec3 m_lightDirection;
    glm::vec3 m_lightColor;

    std::vector<BoundingSphere> m_objectSpheres;
    std::function<bool(const struct Ray&, float&, glm::vec3&)> m_intersectCallback;

public:
    ShadowMapper();

    ShadowMapper(int width, int depth, float cellSize, float groundHeight,
        glm::vec3 lightDirection, glm::vec3 lightColor,
        int strideX = 1, int strideZ = 1);

    // Установка соотношения (1:1, 1:2, 2:1)
    void setStride(int strideX, int strideZ) {
        m_strideX = std::max(1, strideX);
        m_strideZ = std::max(1, strideZ);
        m_totalSamplesX = m_gridWidth / m_strideX;
        m_totalSamplesZ = m_gridDepth / m_strideZ;
        // Пересоздаём сетку с новым размером
        setGrid(m_gridWidth, m_gridDepth, m_cellSize, m_groundHeight);
    }

    void setGrid(int width, int depth, float cellSize, float groundHeight);
    void setLightType(LightType type) { m_lightType = type; }
    void setLightPos(const glm::vec3& pos) { m_lightPos = pos; }
    void setLightDirection(const glm::vec3& direction);
    void setLightColor(const glm::vec3& color);
    void registerObjectBounds(const std::vector<BoundingSphere>& spheres);
    void clearObjectBounds();
    void setIntersectCallback(std::function<bool(const struct Ray&, float&, glm::vec3&)> callback);
    void computeShadows();

    bool isVertexInShadow(const glm::vec3& position);

    float getShadowAtPoint(const glm::vec3& point) const;
    float getShadowAtWorldPos(float x, float z) const;

    const std::vector<std::vector<ShadowSample>>& getShadowGrid() const { return m_shadowGrid; }

    int getTotalSamplesX() const { return m_totalSamplesX; }
    int getTotalSamplesZ() const { return m_totalSamplesZ; }
    int getStrideX() const { return m_strideX; }
    int getStrideZ() const { return m_strideZ; }

    glm::vec3 getVertexPosition(int sampleX, int sampleZ) const;
    bool isInGridBounds(int x, int z) const;

private:
    bool intersectsAnyBoundingSphere(const struct Ray& ray, float& hitDistance);
    float bilinearInterpolate(float x, float z) const;
};