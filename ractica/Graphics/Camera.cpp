#include "../pch.h"
#include "Camera.h"
#include "../Utils/MathUtils.h"

// Конструктор камеры - инициализирует все параметры
Camera::Camera()
    : position(0.0f, CAMERA_DEFAULT_HEIGHT, 5.0f),      // Начальная позиция камеры
    targetPosition(0.0f, 0.0f, 0.0f),                   // Целевая позиция для плавного движения
    front(0.0f, 0.0f, -1.0f),                          // Направление взгляда камеры
    targetFront(0.0f, 0.0f, -1.0f),                    // Целевое направление взгляда
    up(0.0f, 1.0f, 0.0f),                              // Вектор "вверх" для камеры
    right(1.0f, 0.0f, 0.0f),                           // Вектор "вправо" для камеры
    distance(5.0f),                                     // Текущее расстояние до цели
    targetDistance(5.0f),                               // Целевое расстояние до цели
    height(CAMERA_DEFAULT_HEIGHT),                      // Высота камеры над землей
    smoothness(CAMERA_SMOOTHNESS),                      // Коэффициент плавности движения
    minDistance(CAMERA_MIN_DISTANCE),                   // Минимальное расстояние зума
    maxDistance(CAMERA_MAX_DISTANCE),                   // Максимальное расстояние зума
    zoomSpeed(CAMERA_ZOOM_SPEED),                       // Скорость изменения зума
    angle(0.0f),                                        // Текущий угол обзора
    targetAngle(0.0f) {                                 // Целевой угол обзора
}

// Обновляет состояние камеры с плавной интерполяцией
void Camera::update(const glm::vec3& target, float deltaTime) {
    // Вычисляем смещение камеры на основе угла и расстояния
    float offsetX = sin(glm::radians(targetAngle)) * targetDistance;
    float offsetZ = -cos(glm::radians(targetAngle)) * targetDistance;

    // Устанавливаем целевую позицию камеры (позади и выше цели)
    targetPosition = target + glm::vec3(offsetX, height, offsetZ);

    // Направление взгляда камеры на цель
    targetFront = glm::normalize(target - targetPosition);

    // Пересчитываем векторы ориентации камеры
    right = glm::normalize(glm::cross(targetFront, glm::vec3(0.0f, 1.0f, 0.0f)));
    up = glm::normalize(glm::cross(right, targetFront));

    // Плавная интерполяция к целевым значениям
    position = MathUtils::lerp(position, targetPosition, smoothness);
    front = MathUtils::lerp(front, targetFront, smoothness);
    distance = MathUtils::lerp(distance, targetDistance, smoothness);
    angle = MathUtils::lerp(angle, targetAngle, smoothness);
}

// Возвращает матрицу вида (преобразование мировых координат в координаты камеры)
glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(position, position + front, up);
}

// Возвращает матрицу проекции (преобразование 3D в 2D для отображения)
glm::mat4 Camera::getProjectionMatrix(float aspectRatio) const {
    return glm::perspective(
        glm::radians(CAMERA_FOV),      // Угол обзора
        aspectRatio,                   // Соотношение сторон окна
        CAMERA_NEAR_PLANE,             // Ближняя плоскость отсечения
        CAMERA_FAR_PLANE               // Дальняя плоскость отсечения
    );
}

// Обрабатывает зум камеры (прокрутка колесика мыши)
void Camera::zoom(float offset) {
    targetDistance -= offset * zoomSpeed;
    targetDistance = glm::clamp(targetDistance, minDistance, maxDistance);
}

// Вращает камеру вокруг цели (изменение угла обзора)
void Camera::rotate(float angleOffset) {
    targetAngle += angleOffset;
}

// Устанавливает целевую высоту камеры
void Camera::setHeight(float newHeight) {
    height = newHeight;
}

// Устанавливает коэффициент плавности движения камеры
void Camera::setSmoothness(float newSmoothness) {
    smoothness = glm::clamp(newSmoothness, 0.01f, 1.0f);
}

// Сбрасывает камеру в состояние по умолчанию
void Camera::reset() {
    position = glm::vec3(0.0f, CAMERA_DEFAULT_HEIGHT, 5.0f);
    targetPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    front = glm::vec3(0.0f, 0.0f, -1.0f);
    targetFront = glm::vec3(0.0f, 0.0f, -1.0f);
    distance = 5.0f;
    targetDistance = 5.0f;
    height = CAMERA_DEFAULT_HEIGHT;
    angle = 0.0f;
    targetAngle = 0.0f;
}

// Возвращает текущую позицию камеры
glm::vec3 Camera::getPosition() const {
    return position;
}

// Возвращает текущее направление взгляда камеры
glm::vec3 Camera::getFront() const {
    return front;
}

// Возвращает текущее расстояние до цели
float Camera::getDistance() const {
    return distance;
}