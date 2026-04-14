#pragma once
#include "RayTracer.h"
#include <glm/glm.hpp>
#include <vector>
#include <functional>
#include <iostream>

struct Ray;
struct HitInfo;

// Структура для хранения информации о луче для отладки
struct DebugRay {
    glm::vec3 origin;
    glm::vec3 direction;
    glm::vec3 hitPoint;
    float distance;
    bool hit;           // true = пересек объект (красный), false = только пол (синий)
    int rayId;
    int cellX, cellZ;   // Координаты клетки, для которой пускался луч
    int cornerIndex;    // Индекс угла (-1 для центра, 0-3 для углов)

    DebugRay() : origin(0.0f), direction(0.0f), hitPoint(0.0f),
        distance(0.0f), hit(false), rayId(-1), cellX(0), cellZ(0), cornerIndex(-1) {
    }
};

struct ShadowSample {
    bool computed;
    float value;        // 0.0 = в тени, 1.0 = на свету
    glm::vec3 position; // Позиция клетки (центр)
    int cellX, cellZ;   // Координаты клетки в сетке

    // Для градиентного режима
    float cornerShadows[4]; // Тени для 4 углов
    bool useGradient;       // Использовать ли градиент

    ShadowSample() : computed(false), value(1.0f), position(0.0f), cellX(0), cellZ(0), useGradient(false) {
        for (int i = 0; i < 4; i++) cornerShadows[i] = 1.0f;
    }
};

struct BoundingSphere {
    glm::vec3 center;
    float radius;

    BoundingSphere() : center(0.0f), radius(0.0f) {}
    BoundingSphere(const glm::vec3& c, float r) : center(c), radius(r) {}
};

class ShadowMapper {
public:
    // Режимы трассировки теней
    enum ShadowTraceMode {
        TRACE_CENTER = 0,    // Один луч в центр клетки
        TRACE_CORNERS = 1    // Четыре луча по углам клетки + градиент
    };

private:
    int m_gridWidth;        // Количество клеток в игровой сетке
    int m_gridDepth;
    float m_cellSize;
    float m_groundHeight;
    int m_strideX;          // Шаг по X
    int m_strideZ;          // Шаг по Z
    int m_totalCellsX;      // Всего клеток по X
    int m_totalCellsZ;      // Всего клеток по Z

    std::vector<std::vector<ShadowSample>> m_shadowGrid;

    LightType m_lightType;
    glm::vec3 m_lightPos;
    glm::vec3 m_lightDirection;
    glm::vec3 m_lightColor;

    std::vector<BoundingSphere> m_objectSpheres;
    std::function<bool(const struct Ray&, float&, glm::vec3&)> m_intersectCallback;

    // Режим трассировки
    ShadowTraceMode m_shadowTraceMode;

    // ===== ОТЛАДОЧНЫЕ ЛУЧИ =====
    std::vector<DebugRay> m_debugRays;
    bool m_recordDebugRays;
    int m_nextRayId;

    // Вспомогательные методы
    glm::vec3 getCellCenter(int cellX, int cellZ) const;
    glm::vec3 getCornerWorldPosition(int cellX, int cellZ, int cornerIndex) const;
    bool isPointInShadow(const glm::vec3& point, int& hitCellX, int& hitCellZ, float& hitDistance);
    bool traceShadowRay(const glm::vec3& start, const glm::vec3& direction,
        float maxDistance, float& hitDistance, glm::vec3& hitPoint);
    float computeCornerShadow(const glm::vec3& cornerPos, int& hitCellX, int& hitCellZ, float& hitDistance);
    void computeCellGradient(ShadowSample& sample, int cellX, int cellZ);

public:
    ShadowMapper();
    ShadowMapper(int width, int depth, float cellSize, float groundHeight,
        const glm::vec3& lightDirection, const glm::vec3& lightColor,
        LightType lightType, const glm::vec3& lightPos,
        int strideX, int strideZ);

    void setGrid(int width, int depth, float cellSize, float groundHeight);
    void setLightType(LightType type) { m_lightType = type; }
    void setLightPos(const glm::vec3& pos) { m_lightPos = pos; }
    void setLightDirection(const glm::vec3& direction);
    void setLightColor(const glm::vec3& color);
    void registerObjectBounds(const std::vector<BoundingSphere>& spheres);
    void clearObjectBounds();
    void setIntersectCallback(std::function<bool(const struct Ray&, float&, glm::vec3&)> callback);

    // Управление режимом трассировки
    void setShadowTraceMode(ShadowTraceMode mode) { m_shadowTraceMode = mode; }
    ShadowTraceMode getShadowTraceMode() const { return m_shadowTraceMode; }

    // Главный метод - вычисляет тени для всех клеток
    void computeShadows();

    // Получить значение тени для клетки по координатам (обычный режим)
    float getShadowAtCell(int cellX, int cellZ) const;

    // Получить значение тени с градиентом (для режима CORNERS)
    float getShadowAtCellGradient(int cellX, int cellZ, float& outR, float& outG, float& outB) const;
    glm::vec3 getShadowColorAtCell(int cellX, int cellZ) const;

    // Получить значение тени для точки в мире
    float getShadowAtPoint(const glm::vec3& point) const;
    float getShadowAtWorldPos(float x, float z) const;

    // Получить цвет тени для точки (с учётом градиента)
    glm::vec3 getShadowColorAtPoint(const glm::vec3& point) const;

    const std::vector<std::vector<ShadowSample>>& getShadowGrid() const { return m_shadowGrid; }

    int getTotalCellsX() const { return m_totalCellsX; }
    int getTotalCellsZ() const { return m_totalCellsZ; }
    int getStrideX() const { return m_strideX; }
    int getStrideZ() const { return m_strideZ; }

    bool isInGridBounds(int x, int z) const;

    // ===== ОТЛАДОЧНЫЕ МЕТОДЫ ДЛЯ ЛУЧЕЙ =====
    void enableDebugRays(bool enable) { m_recordDebugRays = enable; }
    bool isDebugRaysEnabled() const { return m_recordDebugRays; }
    void clearDebugRays() { m_debugRays.clear(); m_nextRayId = 0; }
    const std::vector<DebugRay>& getDebugRays() const { return m_debugRays; }
    std::vector<DebugRay> getDebugRays() { return m_debugRays; }

private:
    bool intersectsAnyObject(const struct Ray& ray, float& hitDistance, glm::vec3& hitPoint);
    void recordRay(const glm::vec3& origin, const glm::vec3& direction,
        const glm::vec3& hitPoint, float distance, bool hit, int cellX, int cellZ, int cornerIndex = -1);
};