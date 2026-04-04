#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <functional>

// Forward declaration вместо полного include
struct Ray;  // Объявляем, что Ray существует (определен в RayTracer.h)

// Структура для хранения тени на одной вершине сетки
struct ShadowSample {
    bool computed;      // Вычислена ли тень
    float value;        // Значение тени (0.0 = полная тень, 1.0 = полный свет)
    glm::vec3 position; // Позиция вершины

    ShadowSample() : computed(false), value(1.0f), position(0.0f) {}
};

// Структура для bounding volume объекта
struct BoundingSphere {
    glm::vec3 center;
    float radius;

    BoundingSphere() : center(0.0f), radius(0.0f) {}
    BoundingSphere(const glm::vec3& c, float r) : center(c), radius(r) {}
};

class ShadowMapper {
private:
    // Настройки
    int m_gridWidth;
    int m_gridDepth;
    float m_cellSize;
    float m_groundHeight;
    // Данные теней
    std::vector<std::vector<ShadowSample>> m_shadowGrid;
    LightType m_lightType;
    glm::vec3 m_lightPos;
    // Направление солнца
    glm::vec3 m_lightDirection;
    glm::vec3 m_lightColor;

    // Bounding volumes для быстрых тестов
    std::vector<BoundingSphere> m_objectSpheres;

    // Функция для проверки пересечения с объектами (коллбек)
    std::function<bool(const struct Ray&, float&, glm::vec3&)> m_intersectCallback;

public:
    ShadowMapper();
    ShadowMapper(int width,int depth,float cellSize,float groundHeight,glm::vec3 lightDirection,glm::vec3 lightColor);//для полной настроки света
    float computeShadowFactor(const glm::vec3& position);
    // Настройка сетки
    void setGrid(int width, int depth, float cellSize, float groundHeight);
    void setLightType(LightType type) {
        m_lightType = type;
    }

    void setLightPos(const glm::vec3& pos) {
        m_lightPos = pos;
    }
    // Настройка света
    void setLightDirection(const glm::vec3& direction);
    void setLightColor(const glm::vec3& color);

    // Регистрация bounding volumes объектов
    void registerObjectBounds(const std::vector<BoundingSphere>& spheres);
    void clearObjectBounds();

    // Установка коллбека для точного пересечения
    void setIntersectCallback(std::function<bool(const struct Ray&, float&, glm::vec3&)> callback);

    // Основной метод: вычисление теней для всей сетки
    void computeShadows();

    // Получение значения тени в точке (с интерполяцией)
    float getShadowAtPoint(const glm::vec3& point) const;
    float getShadowAtWorldPos(float x, float z) const;

    // Получение сырой сетки теней
    const std::vector<std::vector<ShadowSample>>& getShadowGrid() const { return m_shadowGrid; }

    // Вспомогательные методы
    glm::vec3 getVertexPosition(int x, int z) const;
    bool isInGridBounds(int x, int z) const;

private:
    bool isVertexInShadow(const glm::vec3& position);
    bool intersectsAnyBoundingSphere(const struct Ray& ray, float& hitDistance);
    float bilinearInterpolate(float x, float z) const;
    int worldToGridX(float worldX) const;
    int worldToGridZ(float worldZ) const;
};