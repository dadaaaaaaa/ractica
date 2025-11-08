#pragma once
#include <glm/glm.hpp>
#include "../Core/Constants.h"
#include "../Core/Types.h"

namespace MathUtils {
    // Линейная интерполяция для векторов
    glm::vec3 lerp(const glm::vec3& a, const glm::vec3& b, float t);

    // Линейная интерполяция для скаляров
    float lerp(float a, float b, float t);

    // Ограничение значения в диапазоне [min, max]
    float clamp(float value, float min, float max);

    // Ограничение для целых чисел
    int clamp(int value, int min, int max);

    // Преобразование координат сетки в мировые координаты
    float gridToWorld(int gridCoord, int gridSize);

    // Преобразование мировых координат в координаты сетки
    int worldToGrid(float worldCoord, int gridSize);

    // Преобразование точки сетки в мировые координаты
    glm::vec3 gridPointToWorld(const Point& gridPoint, int gridWidth, int gridDepth);

    // Преобразование мировых координат в точку сетки
    Point worldToGridPoint(const glm::vec3& worldPos, int gridWidth, int gridDepth);

    // Расстояние между двумя точками в сетке
    float gridDistance(const Point& a, const Point& b);

    // Квадрат расстояния (более эффективно для сравнений)
    int gridDistanceSquared(const Point& a, const Point& b);

    // Проверка, находится ли точка в пределах сетки
    bool isInGridBounds(const Point& point, int gridWidth, int gridDepth);

    // Нормализация угла в диапазон [0, 2*PI)
    float normalizeAngle(float angle);

    // Преобразование угла из градусов в радианы
    float degreesToRadians(float degrees);

    // Преобразование угла из радианов в градусы
    float radiansToDegrees(float radians);

    // Случайное число в диапазоне [min, max]
    float randomFloat(float min, float max);

    // Случайное целое число в диапазоне [min, max]
    int randomInt(int min, int max);

    // Округление float до ближайшего целого
    int roundToInt(float value);

    // Проверка приблизительного равенства двух float
    bool approximatelyEqual(float a, float b, float epsilon = 0.001f);

    // Вычисление направления движения на основе угла
    glm::vec3 angleToDirection(float angle);

    // Вычисление угла из направления движения
    float directionToAngle(const glm::vec3& direction);
}