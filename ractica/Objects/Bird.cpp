#include "../pch.h"
#include "Bird.h"
#include "../Utils/MathUtils.h"
#include <iostream>

// Конструктор птицы - инициализирует все параметры
Bird::Bird(glm::vec3 pos, glm::vec3 col, float s, float sp)
    : position(pos), color(col), size(s), speed(sp),
    wingAngle(0.0f), wingSpeed(5.0f + (rand() % 10) * 0.5f),
    direction(glm::vec3(0.0f)), targetDirection(glm::vec3(0.0f)),
    changeDirectionTimer(0.0f), directionChangeInterval(2.0f + (rand() % 5)) {

    // Генерируем случайное начальное направление
    generateRandomDirection();
    targetDirection = direction;

    std::cout << "🐦 Bird created at (" << position.x << ", " << position.y << ", " << position.z
        << ") with speed " << speed << " and wing speed " << wingSpeed << std::endl;
}

// Обновляет состояние птицы (движение и анимация)
void Bird::update(float deltaTime) {
    // Обновляем таймер смены направления
    updateDirectionChange(deltaTime);

    // Плавно интерполируем направление к целевому
    direction = MathUtils::lerp(direction, targetDirection, 0.1f);

    // Обновляем позицию на основе направления и скорости
    position += direction * speed * deltaTime;

    // Обновляем анимацию крыльев
    updateWingAnimation(deltaTime);

    // Применяем гравитацию (легкое опускание)
    applyGravity(deltaTime);

    // Проверяем границы и корректируем позицию если нужно
    checkBoundaries();
}

// Обновляет анимацию взмахов крыльев
// Генерирует случайное направление движения
void Bird::generateRandomDirection() {
    // Случайный угол в горизонтальной плоскости
    float angle = static_cast<float>(rand() % 360) * 3.14159f / 180.0f;

    // Небольшой случайный наклон вверх/вниз
    float verticalAngle = (static_cast<float>(rand() % 60) - 30.0f) * 3.14159f / 180.0f;

    direction.x = cos(angle) * cos(verticalAngle);
    direction.y = sin(verticalAngle);
    direction.z = sin(angle) * cos(verticalAngle);

    // Нормализуем направление
    direction = glm::normalize(direction);
}

// Обновляет таймер смены направления
void Bird::updateDirectionChange(float deltaTime) {
    changeDirectionTimer += deltaTime;

    if (changeDirectionTimer >= directionChangeInterval) {
        generateRandomDirection();
        targetDirection = direction;
        changeDirectionTimer = 0.0f;

        // Случайный интервал до следующей смены направления
        directionChangeInterval = 1.5f + (rand() % 7);
    }
}

// Обновляет анимацию взмахов крыльев
// Применяет легкую гравитацию для естественного движения
void Bird::applyGravity(float deltaTime) {
    // Легкая гравитация для плавного опускания
    direction.y -= 0.1f * deltaTime;

    // Ограничиваем слишком крутое пикирование
    if (direction.y < -0.3f) {
        direction.y = -0.3f;
    }

    // Нормализуем направление после изменения
    direction = glm::normalize(direction);
}

// Проверяет границы и корректирует позицию птицы
void Bird::checkBoundaries() {
    const float BOUNDARY = 15.0f;
    const float MIN_HEIGHT = 2.0f;
    const float MAX_HEIGHT = 6.0f;

    // Проверяем горизонтальные границы
    if (position.x < -BOUNDARY || position.x > BOUNDARY ||
        position.z < -BOUNDARY || position.z > BOUNDARY) {

        // Направляем птицу обратно к центру
        glm::vec3 toCenter = -glm::normalize(position);
        toCenter.y = direction.y; // Сохраняем текущую высоту

        targetDirection = toCenter;
        directionChangeInterval = 1.0f; // Быстрая коррекция
    }

    // Проверяем высоту
    if (position.y < MIN_HEIGHT) {
        position.y = MIN_HEIGHT;
        direction.y = 0.1f; // Направляем немного вверх
    }
    else if (position.y > MAX_HEIGHT) {
        position.y = MAX_HEIGHT;
        direction.y = -0.1f; // Направляем немного вниз
    }
}

// Устанавливает новую позицию птицы
void Bird::setPosition(const glm::vec3& newPosition) {
    position = newPosition;
}

// Устанавливает направление движения
void Bird::setDirection(const glm::vec3& newDirection) {
    direction = glm::normalize(newDirection);
    targetDirection = direction;
}

// Устанавливает скорость движения
void Bird::setSpeed(float newSpeed) {
    speed = glm::clamp(newSpeed, 0.1f, 5.0f);
}

// Устанавливает скорость анимации крыльев
void Bird::setWingSpeed(float newWingSpeed) {
    wingSpeed = glm::clamp(newWingSpeed, 1.0f, 15.0f);
}



// Проверяет, находится ли птица в активном состоянии
bool Bird::isActive() const {
    return speed > 0.1f;
}

// Сбрасывает птицу в начальное состояние
void Bird::reset() {
    wingAngle = 0.0f;
    changeDirectionTimer = 0.0f;
    directionChangeInterval = 2.0f + (rand() % 5);
    generateRandomDirection();
    targetDirection = direction;
}

// Выводит отладочную информацию о птице
void Bird::printDebugInfo() const {
    std::cout << "🐦 Bird Debug Info:" << std::endl;
    std::cout << "  Position: (" << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;
    std::cout << "  Direction: (" << direction.x << ", " << direction.y << ", " << direction.z << ")" << std::endl;
    std::cout << "  Speed: " << speed << std::endl;
    std::cout << "  Wing Angle: " << wingAngle << std::endl;
    std::cout << "  Wing Speed: " << wingSpeed << std::endl;
    std::cout << "  Color: (" << color.r << ", " << color.g << ", " << color.b << ")" << std::endl;
    std::cout << "  Size: " << size << std::endl;
}