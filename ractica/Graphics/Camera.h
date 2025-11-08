#include "../pch.h"
#include "../Core/Constants.h"

// Класс камеры с orbit-контролем для слежения за целью
class Camera {
private:
    // ============================================================================
    // ВЕКТОРЫ ПОЗИЦИИ И ОРИЕНТАЦИИ
    // ============================================================================

    glm::vec3 position;        // Текущая позиция камеры
    glm::vec3 targetPosition;  // Целевая позиция для плавного движения
    glm::vec3 front;           // Текущее направление взгляда
    glm::vec3 targetFront;     // Целевое направление взгляда
    glm::vec3 up;              // Вектор "вверх" в системе координат камеры
    glm::vec3 right;           // Вектор "вправо" в системе координат камеры

    // ============================================================================
    // ПАРАМЕТРЫ КАМЕРЫ
    // ============================================================================

    float distance;            // Текущее расстояние до цели
    float targetDistance;      // Целевое расстояние до цели
    float height;              // Высота камеры над целью
    float smoothness;          // Коэффициент плавности интерполяции
    float minDistance;         // Минимальное расстояние зума
    float maxDistance;         // Максимальное расстояние зума
    float zoomSpeed;           // Скорость изменения зума
    float angle;               // Текущий горизонтальный угол обзора
    float targetAngle;         // Целевой горизонтальный угол обзора

public:
    // ============================================================================
    // КОНСТРУКТОР
    // ============================================================================

    // Создает камеру с параметрами по умолчанию
    Camera();

    // ============================================================================
    // ОСНОВНЫЕ МЕТОДЫ ОБНОВЛЕНИЯ
    // ============================================================================

    // Обновляет состояние камеры с плавной интерполяцией к цели
    void update(const glm::vec3& target, float deltaTime);

    // Изменяет зум камеры (отрицательные значения - приближение, положительные - отдаление)
    void zoom(float offset);

    // Вращает камеру вокруг цели по горизонтали
    void rotate(float angleOffset);

    // ============================================================================
    // МАТРИЦЫ ПРЕОБРАЗОВАНИЯ
    // ============================================================================

    // Возвращает матрицу вида (преобразование мировых координат в координаты камеры)
    glm::mat4 getViewMatrix() const;

    // Возвращает матрицу проекции для заданного соотношения сторон
    glm::mat4 getProjectionMatrix(float aspectRatio) const;

    // ============================================================================
    // ГЕТТЕРЫ
    // ============================================================================

    // Позиция и ориентация
    glm::vec3 getPosition() const { return position; }
    glm::vec3 getFront() const { return front; }
    glm::vec3 getUp() const { return up; }
    glm::vec3 getRight() const { return right; }
    glm::vec3 getTargetPosition() const { return targetPosition; }

    // Параметры камеры
    float getTargetDistance() const { return targetDistance; }
    float getDistance() const { return distance; }
    float getHeight() const { return height; }
    float getAngle() const { return angle; }
    float getSmoothness() const { return smoothness; }

    // ============================================================================
    // СЕТТЕРЫ
    // ============================================================================

    // Установка параметров камеры
    void setHeight(float newHeight) { height = newHeight; }
    void setSmoothness(float newSmoothness) { smoothness = glm::clamp(newSmoothness, 0.01f, 1.0f); }
    void setMinDistance(float minDist) { minDistance = minDist; }
    void setMaxDistance(float maxDist) { maxDistance = maxDist; }
    void setZoomSpeed(float speed) { zoomSpeed = speed; }
    void setTargetDistance(float dist) { targetDistance = glm::clamp(dist, minDistance, maxDistance); }
    void setTargetAngle(float newAngle) { targetAngle = newAngle; }

    // ============================================================================
    // СЛУЖЕБНЫЕ МЕТОДЫ
    // ============================================================================

    // Сбрасывает камеру в состояние по умолчанию
    void reset();

    // Проверяет, находится ли точка в поле зрения камеры
    bool isPointInView(const glm::vec3& point, float aspectRatio) const;
};