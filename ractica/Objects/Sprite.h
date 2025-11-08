#pragma once
#include <glm/glm.hpp>
#include "../Core/Constants.h"

class Sprite {
private:
    glm::vec3 position;
    glm::vec3 color;
    float size;
    float speed;
    bool active;

public:
    // Конструкторы
    Sprite(glm::vec3 pos = glm::vec3(0.0f),
        glm::vec3 col = glm::vec3(1.0f),
        float s = 1.0f,
        float sp = 0.0f);

    // Основные методы
    void update(float deltaTime);
    void move(const glm::vec3& movement);
    void setPosition(const glm::vec3& newPosition);
    void setColor(const glm::vec3& newColor);
    void setSize(float newSize);
    void setSpeed(float newSpeed);

    // Геттеры
    const glm::vec3& getPosition() const { return position; }
    const glm::vec3& getColor() const { return color; }
    float getSize() const { return size; }
    float getSpeed() const { return speed; }
    bool isActive() const { return active; }

    // Управление состоянием
    void activate() { active = true; }
    void deactivate() { active = false; }

    // Утилиты
    float getDistance(const glm::vec3& point) const;
    bool isInRange(const glm::vec3& point, float range) const;

    // Отладочная информация
    void printDebugInfo(const std::string& name = "Sprite") const;
};