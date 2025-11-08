#include "../pch.h"
#include "Sprite.h"
#include <iostream>

Sprite::Sprite(glm::vec3 pos, glm::vec3 col, float s, float sp)
    : position(pos), color(col), size(s), speed(sp), active(true) {
}

void Sprite::update(float deltaTime) {
    if (!active || speed == 0.0f) return;

    // Базовая анимация движения (можно переопределить в наследниках)
    position.x += speed * deltaTime;

    // Пример простой синусоидальной анимации для облаков
    if (size > 0.3f) { // Если это облако (большой размер)
        position.y += sin(position.x * 0.5f) * 0.001f;
    }
}

void Sprite::move(const glm::vec3& movement) {
    if (!active) return;
    position += movement;
}

void Sprite::setPosition(const glm::vec3& newPosition) {
    position = newPosition;
}

void Sprite::setColor(const glm::vec3& newColor) {
    color = newColor;
}

void Sprite::setSize(float newSize) {
    size = newSize;
}

void Sprite::setSpeed(float newSpeed) {
    speed = newSpeed;
}

float Sprite::getDistance(const glm::vec3& point) const {
    return glm::distance(position, point);
}

bool Sprite::isInRange(const glm::vec3& point, float range) const {
    return getDistance(point) <= range;
}

void Sprite::printDebugInfo(const std::string& name) const {
#ifdef _DEBUG
    if (ENABLE_DEBUG_INFO) {
        std::cout << "🎨 " << name << " at ("
            << position.x << ", " << position.y << ", " << position.z
            << "), size: " << size << ", speed: " << speed
            << ", active: " << (active ? "yes" : "no") << std::endl;
    }
#endif
}