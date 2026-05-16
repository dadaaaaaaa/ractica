// ShadowMapper.h
#pragma once
#include "RayTracer.h"
#include <glm/glm.hpp>
#include <vector>
#include <functional>
#include <iostream>

struct Ray;
struct HitInfo;

// Константа для размера подразбиения
const int SHADOW_SUBDIVISION_SIZE = 10;

// Структура для хранения информации о луче для отладки
struct DebugRay {
    glm::vec3 origin;
    glm::vec3 direction;
    glm::vec3 hitPoint;
    float distance;
    bool hit;           // true = пересек объект (тень), false = достиг пола/света
    int rayId;
    int cellX, cellZ;   // Координаты клетки, для которой пускался луч
    int cornerIndex;    // Индекс угла (-1 для центра, 0-3 для углов)
    int subCellX, subCellZ; // Координаты подклетки (-1 если не subdivided)
    bool isSubdivided;  // Является ли луч частью подразбиения
    bool centerCheckPassed; // Для адаптивных режимов: прошел ли проверку центра

    // Тип луча для отладки
    enum RayType {
        RAY_CENTER = 0,         // Центральный луч клетки
        RAY_CORNER = 1,         // Угловой луч клетки
        RAY_SUB_CENTER = 2,     // Центральный луч подклетки
        RAY_SUB_CORNER = 3      // Угловой луч подклетки
    } rayType;

    DebugRay() : origin(0.0f), direction(0.0f), hitPoint(0.0f),
        distance(0.0f), hit(false), rayId(-1), cellX(0), cellZ(0),
        cornerIndex(-1), subCellX(-1), subCellZ(-1), isSubdivided(false),
        centerCheckPassed(false), rayType(RAY_CENTER) {
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

    // Для subdivided режимов - хранение значений подклеток
    std::vector<std::vector<float>> subCellValues; // 10x10 сетка значений теней

    // Для адаптивных режимов: был ли центр освещён
    bool centerWasLit;

    ShadowSample() : computed(false), value(1.0f), position(0.0f), cellX(0), cellZ(0),
        useGradient(false), centerWasLit(false) {
        for (int i = 0; i < 4; i++) cornerShadows[i] = 1.0f;
    }

    // Безопасный доступ к подклеткам
    bool hasSubCells() const { return !subCellValues.empty(); }

    void initSubCells() {
        if (subCellValues.empty()) {
            subCellValues.resize(SHADOW_SUBDIVISION_SIZE,
                std::vector<float>(SHADOW_SUBDIVISION_SIZE, 1.0f));
        }
    }

    float getSubCellValue(int subX, int subZ) const {
        if (subX >= 0 && subX < SHADOW_SUBDIVISION_SIZE &&
            subZ >= 0 && subZ < SHADOW_SUBDIVISION_SIZE &&
            !subCellValues.empty()) {
            return subCellValues[subZ][subX];
        }
        return 1.0f;
    }

    void setSubCellValue(int subX, int subZ, float val) {
        if (subX >= 0 && subX < SHADOW_SUBDIVISION_SIZE &&
            subZ >= 0 && subZ < SHADOW_SUBDIVISION_SIZE &&
            !subCellValues.empty()) {
            subCellValues[subZ][subX] = val;
        }
    }
};

struct BoundingSphere {
    glm::vec3 center;
    float baseRadius;      // Базовый радиус (оригинальный размер объекта)
    float currentRadius;   // Радиус с учётом источника света

    BoundingSphere() : center(0.0f), baseRadius(0.0f), currentRadius(0.0f) {}
    BoundingSphere(const glm::vec3& c, float r) : center(c), baseRadius(r), currentRadius(r) {}

    // Обновить радиус в зависимости от источника света
    void updateRadius(LightType lightType, const glm::vec3& lightPos,
        const glm::vec3& lightDir, float maxDistance = 20.0f) {

        switch (lightType) {
        case LightType::Directional:
            // Направленный свет - резкие тени
            currentRadius = baseRadius * 0.7f;
            break;

        case LightType::Points:
        {
            float distance = glm::distance(center, lightPos);
            // Формула: R = R_base × (1.0 + (dist/maxDist) × 1.5)
            float blurFactor = 1.0f + (distance / maxDistance) * 1.5f;
            blurFactor = glm::clamp(blurFactor, 0.8f, 2.5f);
            currentRadius = baseRadius * blurFactor;
            break;
        }

        case LightType::Spot:
        {
            float distance = glm::distance(center, lightPos);
            // Вектор от объекта к источнику света
            glm::vec3 toLight = glm::normalize(lightPos - center);
            // Угол между направлением к свету и направлением прожектора
            float angleDot = glm::dot(toLight, lightDir);
            float angleFactor = glm::clamp(angleDot, 0.3f, 1.0f);

            // Формула: R = R_base × (1.0 + dist/20) × (1.0 - angle×0.5)
            float blurFactor = 1.0f + (distance / maxDistance) * 1.2f;
            blurFactor *= (1.0f - angleFactor * 0.5f);
            blurFactor = glm::clamp(blurFactor, 0.7f, 2.0f);

            currentRadius = baseRadius * blurFactor;
            break;
        }
        }
    }
};

class ShadowMapper {
public:
    // Режимы трассировки теней
    enum ShadowTraceMode {
        TRACE_CENTER = 0,           // 1 луч в центр клетки (бинарный результат)
        TRACE_CORNERS = 1,          // 4 луча по углам клетки + градиент
        TRACE_CENTER_SUBDIVIDED = 2, // Адаптивный: центр освещён? -> вся клетка свет, иначе разбиение 10x10 с лучами в центры (бинарно)
        TRACE_CORNERS_SUBDIVIDED = 3 // Адаптивный: центр освещён? -> вся клетка свет, иначе разбиение 10x10 с 4 лучами по углам (градиент)
    };
    void updateSpheresRadius(LightType lightType, const glm::vec3& lightPos,
        const glm::vec3& lightDirection) {
        for (auto& sphere : m_objectSpheres) {
            sphere.updateRadius(lightType, lightPos, lightDirection);
        }
    }
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
    glm::vec3 getSubCellCenter(int cellX, int cellZ, int subX, int subZ) const;
    glm::vec3 getSubCellCorner(int cellX, int cellZ, int subX, int subZ, int cornerIndex) const;
    bool isPointInShadow(const glm::vec3& point, int& hitCellX, int& hitCellZ, float& hitDistance);
    bool traceShadowRay(const glm::vec3& start, const glm::vec3& direction,
        float maxDistance, float& hitDistance, glm::vec3& hitPoint);
    float computeCornerShadow(const glm::vec3& cornerPos, int& hitCellX, int& hitCellZ, float& hitDistance);
    void computeCellGradient(ShadowSample& sample, int cellX, int cellZ);

    // Методы для subdivided режимов (адаптивные)
    void computeCellCenterSubdivided(ShadowSample& sample, int cellX, int cellZ);
    void computeCellCornersSubdivided(ShadowSample& sample, int cellX, int cellZ);

    // Инициализация подклеток для всех клеток
    void initAllSubCells();

public:
    ShadowMapper();
    float getShadowAtPointWithSubdivision(const glm::vec3& point, ShadowMapper* otherMapper = nullptr) const;
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
    void setShadowTraceMode(ShadowTraceMode mode);
    ShadowTraceMode getShadowTraceMode() const { return m_shadowTraceMode; }

    // Получить строковое имя режима
    static const char* getModeName(ShadowTraceMode mode);
    const char* getCurrentModeName() const { return getModeName(m_shadowTraceMode); }

    // Главный метод - вычисляет тени для всех клеток
    void computeShadows();

    // Получить значение тени для клетки по координатам
    float getShadowAtCell(int cellX, int cellZ) const;

    // Получить значение тени с градиентом (для режимов с градиентом)
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

    // Получить лучи по типу
    std::vector<DebugRay> getRaysByType(DebugRay::RayType type) const;
    std::vector<DebugRay> getHitRays() const;      // Лучи, которые попали в объекты
    std::vector<DebugRay> getMissRays() const;     // Лучи, которые достигли света
    void setUseSpheres(bool use) { m_useSpheres = use; }
    bool getUseSpheres() const { return m_useSpheres; }
    void setGroundHeight(float height) { m_groundHeight = height; }
private:
    bool m_useSpheres;
    bool intersectsAnyObject(const struct Ray& ray, float& hitDistance, glm::vec3& hitPoint);
    void recordRay(const glm::vec3& origin, const glm::vec3& direction,
        const glm::vec3& hitPoint, float distance, bool hit,
        int cellX, int cellZ, int cornerIndex, int subCellX, int subCellZ,
        bool isSubdivided, bool centerCheckPassed, DebugRay::RayType rayType);

    // Перегруженный метод для удобства
    void recordRay(const glm::vec3& origin, const glm::vec3& direction,
        const glm::vec3& hitPoint, float distance, bool hit,
        int cellX, int cellZ, int cornerIndex = -1, int subCellX = -1, int subCellZ = -1) {
        recordRay(origin, direction, hitPoint, distance, hit, cellX, cellZ,
            cornerIndex, subCellX, subCellZ, false, false, DebugRay::RAY_CENTER);
    }
};